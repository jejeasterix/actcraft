#pragma once
#include <actcraft/core/model.h>

#include <optional>
#include <string_view>
#include <vector>

namespace actcraft::core {

enum class PenShape : std::uint8_t
{
    Free,
    Horizontal,
    Vertical,
    Square,
    Rectangle,
    Circle,
    Ellipse,
    Arc,
    Polyline,
    Arrow,
    Zigzag,
};
std::optional<PenShape> penShapeFromId(std::string_view id); // argument de pen.shape
std::vector<PointF> penShapePoints(PenShape shape, PointF from, PointF to,
                                   const std::vector<PointF>& clicks = {});
PageObject penShapeStroke(const std::vector<PointF>& points, const Outline& pen,
                          bool highlighter); // couche du dessus

} // namespace actcraft::core
