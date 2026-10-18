#include <actcraft/features/pagezoom.h>

#include <algorithm>
#include <cmath>

namespace actcraft::features {

PageZoom::PageZoom(double startZoom) : m_startZoom(startZoom), m_frozenZoom(startZoom) {}

void PageZoom::setZoom(double zoom) noexcept
{
    if (!m_pressed)
        m_frozenZoom = zoom;
}

void PageZoom::press(QPointF viewPoint, Direction direction, std::int64_t nowNs)
{
    m_startZoom = m_frozenZoom;
    m_direction = direction;
    m_anchor = viewPoint;
    m_lastPoint = viewPoint;
    m_panDelta = QPointF();
    m_pressNs = nowNs;
    m_pressed = true;
    m_panning = false;
}

bool PageZoom::move(QPointF viewPoint, std::int64_t /*nowNs*/)
{
    if (!m_pressed)
        return false;
    if (!m_panning) {
        const QPointF d = viewPoint - m_anchor;
        if (std::hypot(d.x(), d.y()) > kPanThreshold) {
            m_panning = true;
            m_frozenZoom = m_startZoom; // le zoom en cours est annulé
        }
    }
    if (m_panning)
        m_panDelta += viewPoint - m_lastPoint;
    m_lastPoint = viewPoint;
    return m_panning;
}

QPointF PageZoom::panDelta() noexcept
{
    const QPointF d = m_panDelta;
    m_panDelta = QPointF();
    return d;
}

void PageZoom::release(std::int64_t nowNs)
{
    if (!m_pressed)
        return;
    m_frozenZoom = zoomAt(nowNs);
    m_pressed = false;
}

double PageZoom::zoomAt(std::int64_t nowNs) const noexcept
{
    if (!m_pressed || m_panning)
        return m_frozenZoom;
    const double seconds = static_cast<double>(nowNs - m_pressNs) / 1e9;
    const double exponent = m_direction == Direction::In ? seconds : -seconds;
    return std::clamp(m_startZoom * std::pow(kRatePerSecond, exponent), kMinZoom, kMaxZoom);
}

QPointF PageZoom::anchor() const noexcept
{
    return m_anchor;
}

bool PageZoom::panning() const noexcept
{
    return m_panning;
}

bool PageZoom::pressed() const noexcept
{
    return m_pressed;
}

bool PageZoom::isDoubleClick(QPointF viewPoint, std::int64_t nowNs)
{
    if (m_hasLastClick) {
        const QPointF d = viewPoint - m_lastClickPoint;
        if (nowNs - m_lastClickNs <= kDoubleClickNs && std::hypot(d.x(), d.y()) <= kDoubleClickPixels) {
            m_hasLastClick = false; // un triple appui ne compte pas deux fois
            return true;
        }
    }
    m_hasLastClick = true;
    m_lastClickPoint = viewPoint;
    m_lastClickNs = nowNs;
    return false;
}

} // namespace actcraft::features
