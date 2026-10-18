#include <actcraft/core/hittest.h>

// Talon L0-S : rempli par L3-T3.

namespace actcraft::core {

bool isAnnotation(const PageObject&) noexcept
{
    return false;
}

bool isSelectable(const PageObject&) noexcept
{
    return false;
}

std::optional<ObjectId> objectAt(const Page&, PointF, double)
{
    return std::nullopt;
}

std::vector<ObjectId> objectsTouching(const Page&, const RectF&)
{
    return {};
}

RectF normalizedRect(PointF, PointF) noexcept
{
    return {};
}

RectF unitedRect(const RectF&, const RectF&) noexcept
{
    return {};
}

bool rectsTouch(const RectF&, const RectF&) noexcept
{
    return false;
}

} // namespace actcraft::core
