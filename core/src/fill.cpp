#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/fill.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/pagesettings.h>

#include <QByteArray>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

namespace actcraft::core {

namespace {

using Polyline = std::vector<PointF>;

struct Outlines
{
    std::vector<Polyline> lines; // en coordonnées de page
    std::vector<bool> closed;
    double halfWidth = 0;
    bool fillable = false; // forme fermée dont l'intérieur se remplit
};

double distanceToSegment(PointF p, PointF a, PointF b) noexcept
{
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double len2 = dx * dx + dy * dy;
    double t = len2 > 0 ? ((p.x - a.x) * dx + (p.y - a.y) * dy) / len2 : 0.0;
    t = std::clamp(t, 0.0, 1.0);
    return std::hypot(p.x - (a.x + t * dx), p.y - (a.y + t * dy));
}

double distanceToLines(const Outlines& o, PointF p) noexcept
{
    double best = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < o.lines.size(); ++i) {
        const Polyline& line = o.lines[i];
        if (line.size() == 1)
            best = std::min(best, std::hypot(p.x - line[0].x, p.y - line[0].y));
        for (std::size_t k = 1; k < line.size(); ++k)
            best = std::min(best, distanceToSegment(p, line[k - 1], line[k]));
        if (o.closed[i] && line.size() > 2)
            best = std::min(best, distanceToSegment(p, line.back(), line.front()));
    }
    return best;
}

// Règle pair-impair sur tous les sous-chemins fermés.
bool insideLines(const Outlines& o, PointF p) noexcept
{
    bool inside = false;
    for (std::size_t i = 0; i < o.lines.size(); ++i) {
        const Polyline& line = o.lines[i];
        if (!o.closed[i] || line.size() < 3)
            continue;
        for (std::size_t a = 0, b = line.size() - 1; a < line.size(); b = a++) {
            const PointF& u = line[a];
            const PointF& v = line[b];
            if ((u.y > p.y) != (v.y > p.y) && p.x < (v.x - u.x) * (p.y - u.y) / (v.y - u.y) + u.x)
                inside = !inside;
        }
    }
    return inside;
}

Polyline mapped(const PageObject& object, const Polyline& points)
{
    Polyline out;
    out.reserve(points.size());
    for (const PointF& p : points)
        out.push_back(object.common.transform.map(p));
    return out;
}

Outlines outlinesOf(const PageObject& object)
{
    Outlines o;
    const auto add = [&](const Polyline& points, bool closed) {
        o.lines.push_back(mapped(object, points));
        o.closed.push_back(closed);
    };
    if (const auto* shape = std::get_if<Shape>(&object)) {
        const bool path = shape->kind == ShapeKind::Path;
        o.fillable = true;
        o.halfWidth = (path || !shape->preset.empty() ? shape->pen.width : shape->outlineWidth) / 2;
        const RectF& b = shape->bounds;
        switch (shape->kind) {
        case ShapeKind::Rectangle:
            add({{b.x, b.y}, {b.x + b.width, b.y}, {b.x + b.width, b.y + b.height}, {b.x, b.y + b.height}},
                true);
            break;
        case ShapeKind::Ellipse: {
            Polyline ring;
            constexpr int kSamples = 64;
            for (int i = 0; i < kSamples; ++i) {
                const double a = 2 * std::numbers::pi * i / kSamples;
                ring.push_back({b.x + b.width / 2 + b.width / 2 * std::cos(a),
                                b.y + b.height / 2 + b.height / 2 * std::sin(a)});
            }
            add(ring, true);
            break;
        }
        case ShapeKind::Polygon:
            add(shape->polygon, true);
            break;
        case ShapeKind::Path:
            for (const Polyline& sub : shape->paths)
                add(sub, !shape->open);
            break;
        }
    } else if (const auto* stroke = std::get_if<Stroke>(&object)) {
        Polyline points;
        double widest = 0;
        for (const StrokePoint& p : stroke->points) {
            points.push_back({p.x, p.y});
            widest = std::max(widest, strokeWidthAt(stroke->baseWidth, p.pressure));
        }
        add(points, false);
        o.halfWidth = widest / 2;
    } else if (const auto* line = std::get_if<Line>(&object)) {
        add(line->points, false);
        o.halfWidth = line->pen.width / 2;
    } else if (const auto* connector = std::get_if<Connector>(&object)) {
        add({connector->firstEnd, connector->secondEnd}, false);
        o.halfWidth = connector->pen.width / 2;
    }
    return o;
}

// Remplace l'objet `id` de la page par une version recalculée ; une seule étape d'annulation.
class ReplaceObjectCommand final : public UndoCommand
{
public:
    ReplaceObjectCommand(std::size_t page, PageObject before, PageObject after)
        : m_page(page), m_before(std::move(before)), m_after(std::move(after))
    {}
    void redo(Document& document, Changes& changes) override { apply(document, changes, m_after); }
    void undo(Document& document, Changes& changes) override { apply(document, changes, m_before); }

private:
    void apply(Document& document, Changes& changes, const PageObject& object)
    {
        Page& page = document.page(m_page);
        const std::size_t index = indexOf(page, m_after.common.id);
        if (index >= page.objects.size())
            return;
        page.objects[index] = object;
        changes.push_back({Change::Kind::ObjectChanged, m_page, object.common.id, boundsOnPage(object)});
    }
    std::size_t m_page;
    PageObject m_before;
    PageObject m_after;
};

class ImageFillCommand final : public UndoCommand
{
public:
    ImageFillCommand(std::size_t page, ObjectId image, std::string pngBytes)
        : m_page(page), m_image(image), m_png(std::move(pngBytes))
    {}
    void redo(Document& document, Changes& changes) override
    {
        Page& page = document.page(m_page);
        const std::size_t index = indexOf(page, m_image);
        auto* image = index < page.objects.size() ? std::get_if<ImageObject>(&page.objects[index]) : nullptr;
        if (image == nullptr)
            return;
        m_before = image->resource;
        m_hadObject = true;
        image->resource =
            document.addResource(QByteArray(m_png.data(), static_cast<qsizetype>(m_png.size())), "png");
        changes.push_back({Change::Kind::ObjectChanged, m_page, m_image, boundsOnPage(page.objects[index])});
    }
    void undo(Document& document, Changes& changes) override
    {
        Page& page = document.page(m_page);
        const std::size_t index = indexOf(page, m_image);
        auto* image = index < page.objects.size() ? std::get_if<ImageObject>(&page.objects[index]) : nullptr;
        if (image == nullptr || !m_hadObject)
            return;
        image->resource = m_before;
        changes.push_back({Change::Kind::ObjectChanged, m_page, m_image, boundsOnPage(page.objects[index])});
    }

private:
    std::size_t m_page;
    ObjectId m_image;
    std::string m_png;
    std::string m_before;
    bool m_hadObject = false;
};

} // namespace

FillHit fillHitAt(const Page& page, PointF point, double tolerance)
{
    for (auto it = page.objects.rbegin(); it != page.objects.rend(); ++it) {
        const PageObject& object = *it;
        if (!isSelectable(object))
            continue;
        if (std::holds_alternative<ImageObject>(object)) {
            const RectF r = boundsOnPage(object);
            if (r.contains(point))
                return {FillTarget::Image, object.common.id};
            continue;
        }
        const Outlines o = outlinesOf(object);
        if (o.lines.empty())
            continue;
        if (distanceToLines(o, point) <= tolerance + o.halfWidth)
            return {FillTarget::Outline, object.common.id};
        if (o.fillable && insideLines(o, point))
            return {FillTarget::ShapeInside, object.common.id};
    }
    return {FillTarget::PageBackground, 0};
}

std::unique_ptr<UndoCommand> makeFillCommand(const Page& page, std::size_t pageIndex, const FillHit& hit,
                                             Argb color)
{
    if (hit.target == FillTarget::PageBackground) {
        Background background = page.background;
        background.fill = solidFill(color);
        return std::make_unique<SetBackgroundCommand>(pageIndex, std::move(background));
    }
    if (hit.target != FillTarget::ShapeInside && hit.target != FillTarget::Outline)
        return nullptr;
    const std::size_t index = indexOf(page, hit.object);
    if (index >= page.objects.size())
        return nullptr;
    PageObject after = page.objects[index];
    const bool inside = hit.target == FillTarget::ShapeInside;
    if (auto* shape = std::get_if<Shape>(&after)) {
        if (inside) {
            shape->fill = color;
            shape->brush = solidFill(color);
        } else {
            shape->outline = color;
            shape->pen.color = color;
        }
    } else if (inside) {
        return nullptr;
    } else if (auto* stroke = std::get_if<Stroke>(&after)) {
        stroke->color = color;
    } else if (auto* line = std::get_if<Line>(&after)) {
        line->pen.color = color;
    } else if (auto* connector = std::get_if<Connector>(&after)) {
        connector->pen.color = color;
    } else {
        return nullptr;
    }
    return std::make_unique<ReplaceObjectCommand>(pageIndex, page.objects[index], std::move(after));
}

std::unique_ptr<UndoCommand> makeImageFillCommand(std::size_t pageIndex, ObjectId image, std::string pngBytes)
{
    return std::make_unique<ImageFillCommand>(pageIndex, image, std::move(pngBytes));
}

} // namespace actcraft::core
