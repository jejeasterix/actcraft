// Talon L11 : rempli par L11-T4.
#include <actcraft/input/touchmodes.h>

namespace actcraft::input {

// Talon L11 : rempli par L11-T4.
std::string_view touchModeId(TouchMode) noexcept
{
    return {};
}

// Talon L11 : rempli par L11-T4.
bool touchModeFromId(std::string_view, TouchMode&) noexcept
{
    return false;
}

// Talon L11 : rempli par L11-T4.
Decision IgnoreTouchPolicy::decide(const Sample&, const RouterState&) const
{
    return {};
}

// Talon L11 : rempli par L11-T4.
Decision PenTouchPolicy::decide(const Sample&, const RouterState&) const
{
    return {};
}

// Talon L11 : rempli par L11-T4.
Decision SelectTouchPolicy::decide(const Sample&, const RouterState&) const
{
    return {};
}

// Talon L11 : rempli par L11-T4.
std::unique_ptr<RoutingPolicy> makeTouchPolicy(TouchMode)
{
    return std::make_unique<FullTouchPolicy>();
}

} // namespace actcraft::input
