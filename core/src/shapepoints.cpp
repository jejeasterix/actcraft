#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/shapepoints.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

namespace actcraft::core {

namespace {

struct Contour
{
    std::vector<PointF> points;
    bool closed = true;
};

// Contours de la forme en coordonnées de l'objet. Rectangle : quatre coins ; ellipse : huit points
// (cardinaux et diagonaux) ; polygone : ses sommets ; chemin : ses sous-chemins.
std::vector<Contour> contoursOf(const Shape& s)
{
    switch (s.kind) {
    case ShapeKind::Rectangle: {
        const RectF b = s.bounds;
        return {{{{b.x, b.y}, {b.x + b.width, b.y}, {b.x + b.width, b.y + b.height}, {b.x, b.y + b.height}},
                 true}};
    }
    case ShapeKind::Ellipse: {
        const PointF c = s.bounds.center();
        std::vector<PointF> pts;
        for (int i = 0; i < 8; ++i) {
            const double a = i * std::numbers::pi / 4;
            pts.push_back({c.x + std::cos(a) * s.bounds.width / 2, c.y + std::sin(a) * s.bounds.height / 2});
        }
        return {{pts, true}};
    }
    case ShapeKind::Polygon:
        return {{s.polygon, true}};
    case ShapeKind::Path: {
        std::vector<Contour> out;
        for (const auto& path : s.paths)
            out.push_back({path, !s.open});
        return out;
    }
    }
    return {};
}

std::size_t midpointCount(const Contour& c)
{
    const std::size_t n = c.points.size();
    if (n < 2)
        return 0;
    return c.closed ? n : n - 1;
}

PointF midpoint(const Contour& c, std::size_t i)
{
    const PointF a = c.points[i];
    const PointF b = c.points[(i + 1) % c.points.size()];
    return {(a.x + b.x) / 2, (a.y + b.y) / 2};
}

bool invert(const Transform& t, Transform& out)
{
    const double det = t.m11 * t.m22 - t.m12 * t.m21;
    if (det == 0 || !std::isfinite(det))
        return false;
    out.m11 = t.m22 / det;
    out.m12 = -t.m12 / det;
    out.m21 = -t.m21 / det;
    out.m22 = t.m11 / det;
    out.dx = -(out.m11 * t.dx + out.m21 * t.dy);
    out.dy = -(out.m12 * t.dx + out.m22 * t.dy);
    return true;
}

} // namespace

std::vector<PointF> shapePoints(const PageObject& object)
{
    const auto* shape = std::get_if<Shape>(&body(object));
    if (shape == nullptr)
        return {};
    std::vector<PointF> out;
    for (const Contour& c : contoursOf(*shape)) {
        for (const PointF& p : c.points)
            out.push_back(object.common.transform.map(p));
        for (std::size_t i = 0; i < midpointCount(c); ++i)
            out.push_back(object.common.transform.map(midpoint(c, i)));
    }
    return out;
}

MoveShapePointCommand::MoveShapePointCommand(std::size_t page, ObjectId shape, std::size_t index, PointF to)
    : m_page(page), m_shape(shape), m_index(index), m_to(to)
{}

void MoveShapePointCommand::redo(Document& document, Changes& changes)
{
    m_before.reset();
    Page& page = document.page(m_page);
    const std::size_t rank = indexOf(page, m_shape);
    if (rank == page.objects.size())
        return;
    PageObject& object = page.objects[rank];
    auto* shape = std::get_if<Shape>(&body(object));
    Transform inverse;
    if (shape == nullptr || object.common.locked || !invert(object.common.transform, inverse))
        return;

    std::vector<Contour> contours = contoursOf(*shape);
    std::size_t remaining = m_index;
    std::size_t target = contours.size();
    bool isMidpoint = false;
    for (std::size_t c = 0; c < contours.size() && target == contours.size(); ++c) {
        const std::size_t n = contours[c].points.size();
        const std::size_t total = n + midpointCount(contours[c]);
        if (remaining < total) {
            target = c;
            isMidpoint = remaining >= n;
            remaining = isMidpoint ? remaining - n : remaining;
        } else {
            remaining -= total;
        }
    }
    if (target == contours.size())
        return;

    const PointF local = inverse.map(m_to);
    Contour& contour = contours[target];
    if (isMidpoint)
        contour.points.insert(contour.points.begin() + static_cast<std::ptrdiff_t>(remaining + 1), local);
    else
        contour.points[remaining] = local;

    m_before = object;
    const RectF before = boundsOnPage(object);
    if (shape->kind == ShapeKind::Path) {
        shape->paths[target] = contour.points;
    } else {
        shape->kind = ShapeKind::Polygon;
        shape->polygon = contour.points;
        shape->paths.clear();
        shape->preset.clear();
        shape->parameters.clear();
        double x0 = local.x, y0 = local.y, x1 = local.x, y1 = local.y;
        for (const PointF& p : shape->polygon) {
            x0 = std::min(x0, p.x);
            y0 = std::min(y0, p.y);
            x1 = std::max(x1, p.x);
            y1 = std::max(y1, p.y);
        }
        shape->bounds = {x0, y0, x1 - x0, y1 - y0};
    }
    changes.push_back(
        Change{Change::Kind::ObjectChanged, m_page, m_shape, unitedRect(before, boundsOnPage(object))});
}

void MoveShapePointCommand::undo(Document& document, Changes& changes)
{
    if (!m_before)
        return;
    Page& page = document.page(m_page);
    const std::size_t rank = indexOf(page, m_shape);
    if (rank == page.objects.size())
        return;
    const RectF after = boundsOnPage(page.objects[rank]);
    page.objects[rank] = *m_before;
    changes.push_back(Change{Change::Kind::ObjectChanged, m_page, m_shape,
                             unitedRect(after, boundsOnPage(page.objects[rank]))});
}

} // namespace actcraft::core
