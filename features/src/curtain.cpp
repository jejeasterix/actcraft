#include "nearestside.h"

#include <actcraft/features/curtain.h>

#include <algorithm>

namespace actcraft::features {

Curtain::Curtain(QSizeF area) : m_area(area) {}

void Curtain::setArea(QSizeF area)
{
    m_area = area; // les fractions découvertes restent : les proportions sont gardées
}

void Curtain::reset(Start start, double distance)
{
    m_top = m_bottom = m_left = m_right = 0;
    m_edge = Edge::None;
    const double w = m_area.width();
    const double h = m_area.height();
    const auto fraction = [](double d, double size) {
        return size > 0 ? std::clamp(d / size, 0.0, 1.0) : 0.0;
    };
    switch (start) {
    case Start::Full:
        break;
    case Start::Top:
        m_top = fraction(distance, h);
        break;
    case Start::Bottom:
        m_bottom = fraction(distance, h);
        break;
    case Start::Left:
        m_left = fraction(distance, w);
        break;
    case Start::Right:
        m_right = fraction(distance, w);
        break;
    }
}

bool Curtain::press(QPointF point)
{
    const double w = m_area.width();
    const double h = m_area.height();
    if (w <= 0 || h <= 0)
        return false;
    const double top = m_top * h, bottom = (1 - m_bottom) * h, left = m_left * w, right = (1 - m_right) * w;
    const std::optional<Side> side = nearestSide(point, left, top, right, bottom, kGrabMargin,
                                                 {Side::Top, Side::Bottom, Side::Left, Side::Right});
    m_edge = Edge::None;
    if (side) {
        switch (*side) {
        case Side::Left:
            m_edge = Edge::Left;
            break;
        case Side::Right:
            m_edge = Edge::Right;
            break;
        case Side::Top:
            m_edge = Edge::Top;
            break;
        case Side::Bottom:
            m_edge = Edge::Bottom;
            break;
        }
    }
    return m_edge != Edge::None;
}

void Curtain::move(QPointF point)
{
    const double w = m_area.width();
    const double h = m_area.height();
    if (w <= 0 || h <= 0)
        return;
    switch (m_edge) {
    case Edge::None:
        break;
    case Edge::Top:
        m_top = std::clamp(point.y() / h, 0.0, 1 - m_bottom);
        break;
    case Edge::Bottom:
        m_bottom = std::clamp((h - point.y()) / h, 0.0, 1 - m_top);
        break;
    case Edge::Left:
        m_left = std::clamp(point.x() / w, 0.0, 1 - m_right);
        break;
    case Edge::Right:
        m_right = std::clamp((w - point.x()) / w, 0.0, 1 - m_left);
        break;
    }
}

void Curtain::release()
{
    m_edge = Edge::None;
}

bool Curtain::dragging() const noexcept
{
    return m_edge != Edge::None;
}

QRectF Curtain::covered() const noexcept
{
    const double w = m_area.width();
    const double h = m_area.height();
    const QRectF r(m_left * w, m_top * h, w * (1 - m_left - m_right), h * (1 - m_top - m_bottom));
    return (r.width() <= 0 || r.height() <= 0) ? QRectF() : r;
}

double Curtain::coverage() const noexcept
{
    const double area = m_area.width() * m_area.height();
    if (area <= 0)
        return 0;
    const QRectF r = covered();
    return r.width() * r.height() / area;
}

} // namespace actcraft::features
