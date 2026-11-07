// Talon L11 : rempli par L11-T7.
#include <actcraft/input/twouserpolicy.h>

namespace actcraft::input {

// Talon L11 : rempli par L11-T7.
TwoUserPolicy::TwoUserPolicy(QRectF secondUserArea) : m_area(secondUserArea) {}

// Talon L11 : rempli par L11-T7.
void TwoUserPolicy::setSecondUserArea(QRectF area)
{
    m_area = area;
}

// Talon L11 : rempli par L11-T7.
Decision TwoUserPolicy::decide(const Sample&, const RouterState&) const
{
    return {};
}

// Talon L11 : rempli par L11-T7.
std::uint64_t TwoUserPolicy::firstStylusDevice() const noexcept
{
    return m_firstStylus;
}

} // namespace actcraft::input
