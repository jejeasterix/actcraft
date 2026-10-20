#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/objectedits.h>

#include <algorithm>
#include <cmath>

namespace actcraft::core {

namespace {

double distanceToSegment(PointF p, PointF a, PointF b) noexcept
{
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double len2 = dx * dx + dy * dy;
    double t = 0;
    if (len2 > 0)
        t = std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / len2, 0.0, 1.0);
    return std::hypot(p.x - (a.x + t * dx), p.y - (a.y + t * dy));
}

bool strokeHit(const PageObject& object, const Stroke& stroke, PointF point, double tolerance) noexcept
{
    const auto& pts = stroke.points;
    if (pts.empty())
        return false;
    const auto mapped = [&](const StrokePoint& s) { return object.common.transform.map({s.x, s.y}); };
    const double scale = lengthScale(object.common.transform); // épaisseur visible d'un trait transformé
    if (pts.size() == 1) {
        const double half = strokeWidthAt(stroke.baseWidth, pts[0].pressure) * scale / 2;
        const PointF m = mapped(pts[0]);
        return std::hypot(point.x - m.x, point.y - m.y) <= half + tolerance;
    }
    for (std::size_t i = 1; i < pts.size(); ++i) {
        const double half =
            strokeWidthAt(stroke.baseWidth, std::max(pts[i - 1].pressure, pts[i].pressure)) * scale / 2;
        if (distanceToSegment(point, mapped(pts[i - 1]), mapped(pts[i])) <= half + tolerance)
            return true;
    }
    return false;
}

} // namespace

bool isAnnotation(const PageObject& object) noexcept
{
    return std::get_if<Stroke>(&body(object)) != nullptr && object.common.layer == Layer::Top;
}

bool isSelectable(const PageObject& object) noexcept
{
    return object.common.visible;
}

std::optional<ObjectId> objectAt(const Page& page, PointF point, double tolerance)
{
    for (auto it = page.objects.rbegin(); it != page.objects.rend(); ++it) {
        const PageObject& object = *it;
        if (!isSelectable(object))
            continue;
        if (const auto* stroke = std::get_if<Stroke>(&body(object))) {
            if (strokeHit(object, *stroke, point, tolerance))
                return object.common.id;
            continue;
        }
        const RectF b = boundsOnPage(object);
        const RectF grown{b.x - tolerance, b.y - tolerance, b.width + 2 * tolerance,
                          b.height + 2 * tolerance};
        if (grown.contains(point))
            return object.common.id;
    }
    return std::nullopt;
}

std::vector<ObjectId> objectsTouching(const Page& page, const RectF& rect)
{
    std::vector<ObjectId> ids;
    for (const PageObject& object : page.objects)
        if (isSelectable(object) && rectsTouch(boundsOnPage(object), rect))
            ids.push_back(object.common.id);
    return ids;
}

RectF normalizedRect(PointF a, PointF b) noexcept
{
    return {std::min(a.x, b.x), std::min(a.y, b.y), std::abs(a.x - b.x), std::abs(a.y - b.y)};
}

RectF unitedRect(const RectF& a, const RectF& b) noexcept
{
    const auto empty = [](const RectF& r) { return r.width == 0 && r.height == 0; };
    if (empty(a))
        return b;
    if (empty(b))
        return a;
    const double x0 = std::min(a.x, b.x);
    const double y0 = std::min(a.y, b.y);
    const double x1 = std::max(a.x + a.width, b.x + b.width);
    const double y1 = std::max(a.y + a.height, b.y + b.height);
    return {x0, y0, x1 - x0, y1 - y0};
}

bool rectsTouch(const RectF& a, const RectF& b) noexcept
{
    return a.x <= b.x + b.width && b.x <= a.x + a.width && a.y <= b.y + b.height && b.y <= a.y + a.height;
}

} // namespace actcraft::core
