#include <actcraft/core/erasegeometry.h>

// Talon L0-S : rempli par L3-T3.

namespace actcraft::core {

StrokeCut cutByDisk(const std::vector<StrokePoint>&, PointF, double)
{
    return {};
}

StrokeCut cutBySweep(const std::vector<StrokePoint>&, PointF, PointF, double)
{
    return {};
}

std::vector<StrokePoint> strokePointsOnPage(const PageObject&)
{
    return {};
}

} // namespace actcraft::core
