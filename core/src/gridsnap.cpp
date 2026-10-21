#include <actcraft/core/gridsnap.h>

namespace actcraft::core {

// Talon L4-T1 : rempli par L4-T5.
std::optional<PointF> nearestGridPoint(const Grid&, PointF, double, double) noexcept
{
    return std::nullopt;
}

PointF snapAnchor(const PageObject&)
{
    return {};
}

PointF snapDelta(const Page&, const std::vector<ObjectId>&, PointF delta) noexcept
{
    return delta;
}

} // namespace actcraft::core
