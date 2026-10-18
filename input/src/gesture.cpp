#include <actcraft/input/gesture.h>

// Talon L0-S : rempli par L3-T5.

namespace actcraft::input {

void TwoFingerGesture::begin(Point2, Point2)
{
    static_cast<void>(m_anchor);
    static_cast<void>(m_distance);
}

GestureStep TwoFingerGesture::update(Point2, Point2) const
{
    return {};
}

bool TwoFingerGesture::active() const noexcept
{
    return m_active;
}

void TwoFingerGesture::end() noexcept {}

} // namespace actcraft::input
