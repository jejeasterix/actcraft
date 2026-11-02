#include <actcraft/core/penmodifiers.h>

namespace actcraft::core {

// Talon L5-T1 : rempli par L5-T11.
std::optional<PenShape> penShapeFromId(std::string_view id)
{
    static_cast<void>(id);
    return std::nullopt;
}

// Talon L5-T1 : rempli par L5-T11.
std::vector<PointF> penShapePoints(PenShape shape, PointF from, PointF to, const std::vector<PointF>& clicks)
{
    static_cast<void>(shape);
    static_cast<void>(from);
    static_cast<void>(to);
    static_cast<void>(clicks);
    return {};
}

// Talon L5-T1 : rempli par L5-T11.
PageObject penShapeStroke(const std::vector<PointF>& points, const Outline& pen, bool highlighter)
{
    static_cast<void>(points);
    static_cast<void>(pen);
    static_cast<void>(highlighter);
    return PageObject{Stroke{}};
}

} // namespace actcraft::core
