#include "curtaincontroller.h"

#include <cmath>

namespace actcraft::ui {

CurtainController::CurtainController(QObject* parent) : QObject(parent), m_curtain(QSizeF()) {}

bool CurtainController::active() const noexcept
{
    return m_active;
}

void CurtainController::setActive(bool active)
{
    if (m_active == active) {
        return;
    }
    m_active = active;
    emit activeChanged();
}

QColor CurtainController::color() const noexcept
{
    return m_color;
}

void CurtainController::setColor(const QColor& color)
{
    if (!color.isValid() || m_color == color) {
        return;
    }
    m_color = color;
    emit colorChanged();
}

QSizeF CurtainController::area() const noexcept
{
    return m_area;
}

void CurtainController::setArea(QSizeF area)
{
    if (m_area == area) {
        return;
    }
    m_area = area;
    m_curtain.setArea(area);
    emit areaChanged();
    emit changed();
}

QRectF CurtainController::covered() const noexcept
{
    return m_curtain.covered();
}

qreal CurtainController::coverage() const noexcept
{
    return std::round(m_curtain.coverage() * 100.0) / 100.0;
}

bool CurtainController::dragging() const noexcept
{
    return m_curtain.dragging();
}

bool CurtainController::press(QPointF point)
{
    const bool grabbed = m_curtain.press(point);
    emit changed();
    return grabbed;
}

void CurtainController::move(QPointF point)
{
    if (!m_curtain.dragging()) {
        return;
    }
    m_curtain.move(point);
    emit changed();
}

void CurtainController::release()
{
    m_curtain.release();
    emit changed();
}

void CurtainController::reset(int start, qreal distance)
{
    using Start = features::Curtain::Start;
    Start where = Start::Full;
    switch (start) {
    case 1:
        where = Start::Top;
        break;
    case 2:
        where = Start::Bottom;
        break;
    case 3:
        where = Start::Left;
        break;
    case 4:
        where = Start::Right;
        break;
    default:
        break;
    }
    m_curtain.reset(where, distance);
    emit changed();
}

void CurtainController::toggle()
{
    if (!m_active) {
        m_curtain.reset(features::Curtain::Start::Full, 0);
        emit changed();
    }
    setActive(!m_active);
}

} // namespace actcraft::ui
