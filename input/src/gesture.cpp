#include <actcraft/input/gesture.h>

#include <cmath>

namespace actcraft::input {

namespace {

Point2 midpoint(Point2 a, Point2 b)
{
    Point2 out;
    out.x = (a.x + b.x) / 2;
    out.y = (a.y + b.y) / 2;
    return out;
}

double spread(Point2 a, Point2 b)
{
    return std::hypot(b.x - a.x, b.y - a.y);
}

} // namespace

void TwoFingerGesture::begin(Point2 a, Point2 b)
{
    m_anchor = midpoint(a, b);
    m_distance = spread(a, b);
    m_active = true;
}

GestureStep TwoFingerGesture::update(Point2 a, Point2 b) const
{
    GestureStep step;
    step.center = midpoint(a, b);
    if (!m_active) {
        step.anchor = step.center; // sans begin : rien ne bouge
        return step;
    }
    step.anchor = m_anchor;
    if (m_distance > 0) {
        // Écart relatif sous la zone morte (D12, H3-14) : défilement seul, le zoom ne bouge pas.
        const double scale = spread(a, b) / m_distance;
        step.scale = std::abs(scale - 1) < kPinchDeadZone ? 1.0 : scale;
    }
    return step;
}

bool TwoFingerGesture::active() const noexcept
{
    return m_active;
}

void TwoFingerGesture::end() noexcept
{
    m_active = false;
}

} // namespace actcraft::input
