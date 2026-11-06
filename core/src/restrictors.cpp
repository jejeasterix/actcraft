#include <actcraft/core/restrictors.h>

// Version neutre pour le test rouge : chaque fonction rend la valeur voulue telle quelle.
namespace actcraft::core {

PointF allowedTravel(const Page&, const PageObject&, PointF wanted)
{
    return wanted;
}

PointF projectOnTrack(const Page&, const PageObject&, PointF point)
{
    return point;
}

PointF landingPoint(const Page&, const PageObject&, PointF dropped)
{
    return dropped;
}

bool sizeHandlesShown(const PageObject&) noexcept
{
    return true;
}

double allowedTurn(const PageObject&, double, double wantedDegrees) noexcept
{
    return wantedDegrees;
}

PointF pivotOf(const Page&, const PageObject&)
{
    return {};
}

PointF stopAtBlockers(const Page&, const std::vector<ObjectId>&, PointF wanted)
{
    return wanted;
}

} // namespace actcraft::core
