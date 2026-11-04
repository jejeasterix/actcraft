#include <actcraft/core/penmodifiers.h>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace actcraft::core {

namespace {

constexpr int kEllipseSegments = 64;
constexpr double kZigzagStep = 16;
constexpr double kArrowHead = 20;
constexpr double kArrowAngle = 30.0 * std::numbers::pi / 180.0;

std::vector<PointF> ellipsePoints(double cx, double cy, double rx, double ry)
{
    std::vector<PointF> out;
    out.reserve(kEllipseSegments + 1);
    for (int i = 0; i < kEllipseSegments; ++i) {
        const double t = 2 * std::numbers::pi * i / kEllipseSegments;
        out.push_back({cx + rx * std::cos(t), cy + ry * std::sin(t)});
    }
    out.push_back(out.front()); // fermé exactement
    return out;
}

std::vector<PointF> boxPoints(PointF a, PointF b)
{
    return {a, {b.x, a.y}, b, {a.x, b.y}, a};
}

PointF squareCorner(PointF from, PointF to)
{
    const double dx = to.x - from.x;
    const double dy = to.y - from.y;
    const double side = std::max(std::abs(dx), std::abs(dy));
    return {from.x + (dx < 0 ? -side : side), from.y + (dy < 0 ? -side : side)};
}

std::vector<PointF> zigzagPoints(PointF from, PointF to)
{
    const double dx = to.x - from.x;
    if (dx == 0)
        return {from, to};
    const double dir = dx < 0 ? -1 : 1;
    const int steps = static_cast<int>(std::ceil(std::abs(dx) / kZigzagStep));
    std::vector<PointF> out{from};
    for (int i = 1; i <= steps; ++i) {
        const double x = i == steps ? to.x : from.x + dir * kZigzagStep * i;
        out.push_back({x, (i % 2 == 1) ? to.y : from.y});
    }
    return out;
}

std::vector<PointF> arrowPoints(PointF from, PointF to)
{
    const double dx = from.x - to.x;
    const double dy = from.y - to.y;
    const double len = std::hypot(dx, dy);
    if (len == 0)
        return {from, to};
    const double ux = dx / len;
    const double uy = dy / len;
    const auto wing = [&](double angle) {
        const double c = std::cos(angle);
        const double s = std::sin(angle);
        return PointF{to.x + kArrowHead * (ux * c - uy * s), to.y + kArrowHead * (ux * s + uy * c)};
    };
    return {from, to, wing(kArrowAngle), to, wing(-kArrowAngle)};
}

} // namespace

std::optional<PenShape> penShapeFromId(std::string_view id)
{
    struct Entry
    {
        std::string_view id;
        PenShape shape;
    };
    static constexpr Entry kEntries[] = {
        {"free", PenShape::Free},           {"horizontal", PenShape::Horizontal},
        {"vertical", PenShape::Vertical},   {"square", PenShape::Square},
        {"rectangle", PenShape::Rectangle}, {"circle", PenShape::Circle},
        {"ellipse", PenShape::Ellipse},     {"arc", PenShape::Arc},
        {"polyline", PenShape::Polyline},   {"arrow", PenShape::Arrow},
        {"zigzag", PenShape::Zigzag},
    };
    for (const auto& e : kEntries)
        if (e.id == id)
            return e.shape;
    return std::nullopt;
}

std::vector<PointF> penShapePoints(PenShape shape, PointF from, PointF to, const std::vector<PointF>& clicks)
{
    switch (shape) {
    case PenShape::Free:
        return {from, to};
    case PenShape::Horizontal:
        return {from, {to.x, from.y}};
    case PenShape::Vertical:
        return {from, {from.x, to.y}};
    case PenShape::Square:
        return boxPoints(from, squareCorner(from, to));
    case PenShape::Rectangle:
        return boxPoints(from, to);
    case PenShape::Circle: {
        const PointF corner = squareCorner(from, to);
        return ellipsePoints((from.x + corner.x) / 2, (from.y + corner.y) / 2,
                             std::abs(corner.x - from.x) / 2, std::abs(corner.y - from.y) / 2);
    }
    case PenShape::Ellipse:
        return ellipsePoints((from.x + to.x) / 2, (from.y + to.y) / 2, std::abs(to.x - from.x) / 2,
                             std::abs(to.y - from.y) / 2);
    case PenShape::Arc: {
        // Demi-ellipse supérieure inscrite dans la boîte, de gauche à droite.
        const double cx = (from.x + to.x) / 2;
        const double top = std::min(from.y, to.y);
        const double ry = std::abs(to.y - from.y) / 2;
        const double cy = top + ry;
        const double rx = std::abs(to.x - from.x) / 2;
        const double left = std::min(from.x, to.x);
        std::vector<PointF> out;
        for (int i = 0; i <= kEllipseSegments / 2; ++i) {
            const double t = std::numbers::pi * (1.0 - static_cast<double>(i) / (kEllipseSegments / 2));
            out.push_back({cx + rx * std::cos(t), cy - ry * std::sin(t)});
        }
        out.front() = {left, cy};
        out.back() = {left + 2 * rx, cy};
        return out;
    }
    case PenShape::Polyline: {
        std::vector<PointF> out{from};
        out.insert(out.end(), clicks.begin(), clicks.end());
        out.push_back(to);
        return out;
    }
    case PenShape::Arrow:
        return arrowPoints(from, to);
    case PenShape::Zigzag:
        return zigzagPoints(from, to);
    }
    return {from, to};
}

PageObject penShapeStroke(const std::vector<PointF>& points, const Outline& pen, bool highlighter)
{
    Stroke stroke;
    stroke.points.reserve(points.size());
    for (const auto& p : points)
        stroke.points.push_back({p.x, p.y, 0.5f}); // pression 0,5 : largeur rendue = largeur de base
    stroke.baseWidth = pen.width;
    stroke.color = pen.color;
    stroke.kind = highlighter ? StrokeKind::Highlighter : StrokeKind::Pen;
    stroke.cap = pen.cap;
    PageObject object{std::move(stroke)};
    object.common.layer = Layer::Top;
    return object;
}

} // namespace actcraft::core
