#include <actcraft/core/shapegeometry.h>

namespace actcraft::core {

// Talon L4-T1 : rempli par L4-T3.
const std::vector<ShapePreset>& shapeCatalog()
{
    static const std::vector<ShapePreset> catalog;
    return catalog;
}

const ShapePreset* findShapePreset(std::string_view) noexcept
{
    return nullptr;
}

RectF dragBox(PointF, PointF, bool, bool) noexcept
{
    return {};
}

std::pair<PointF, PointF> dragLine(PointF, PointF, const ShapePreset&, bool, bool) noexcept
{
    return {};
}

PageObject makeShapeObject(std::string_view, const RectF&, const ShapeStyle&)
{
    return PageObject{Shape{}};
}

PageObject makeLineObject(std::string_view, PointF, PointF, const ShapeStyle&)
{
    return PageObject{Line{}};
}

bool rebuildShape(Shape&)
{
    return false;
}

std::vector<PointF> smartPoints(const Shape&)
{
    return {};
}

std::vector<double> dragSmartPoint(const Shape&, std::size_t, PointF)
{
    return {};
}

} // namespace actcraft::core
