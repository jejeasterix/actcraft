#include "nearestside.h"

#include <actcraft/features/spotlight.h>

#include <algorithm>

namespace actcraft::features {

Spotlight::Spotlight(QSizeF area) : m_area(area)
{
    m_frame = defaultFrame();
}

QRectF Spotlight::defaultFrame() const
{
    QRectF r(0, 0, kDefaultSize, kDefaultSize);
    r.moveCenter(QPointF(m_area.width() / 2, m_area.height() / 2));
    return r;
}

void Spotlight::setArea(QSizeF area)
{
    m_area = area;
    keepInside();
}

void Spotlight::setShape(Shape shape)
{
    m_shape = shape;
    m_grab = Grab::None;
}

Spotlight::Shape Spotlight::shape() const noexcept
{
    return m_shape;
}

void Spotlight::setFrame(QRectF frame)
{
    m_frame = frame.isEmpty() ? defaultFrame() : frame;
    keepInside();
}

QRectF Spotlight::frame() const noexcept
{
    return m_frame;
}

bool Spotlight::hidesInside() const noexcept
{
    return m_shape == Shape::SolidCircle || m_shape == Shape::SolidSquare;
}

void Spotlight::keepInside()
{
    if (m_area.width() <= 0 || m_area.height() <= 0)
        return;
    QPointF c = m_frame.center();
    c.setX(std::clamp(c.x(), 0.0, m_area.width()));
    c.setY(std::clamp(c.y(), 0.0, m_area.height()));
    m_frame.moveCenter(c);
}

bool Spotlight::press(QPointF point)
{
    m_grab = Grab::None;
    if (m_shape == Shape::Off || m_area.width() <= 0 || m_area.height() <= 0)
        return false;

    const QRectF f = m_frame;
    const std::optional<Side> side = nearestSide(point, f.left(), f.top(), f.right(), f.bottom(), kRimMargin,
                                                 {Side::Left, Side::Right, Side::Top, Side::Bottom});
    if (side) {
        switch (*side) {
        case Side::Left:
            m_grab = Grab::Left;
            break;
        case Side::Right:
            m_grab = Grab::Right;
            break;
        case Side::Top:
            m_grab = Grab::Top;
            break;
        case Side::Bottom:
            m_grab = Grab::Bottom;
            break;
        }
    } else {
        // Décision : pour un cache plein comme pour un spot, le pourtour l'emporte sur l'intérieur (il est
        // testé d'abord) ; l'intérieur d'un cache plein ne déplace que hors de la bande du pourtour.
        const bool inside = f.contains(point);
        if (inside == hidesInside())
            m_grab = Grab::Move;
    }
    m_last = point;
    return m_grab != Grab::None;
}

void Spotlight::move(QPointF point)
{
    switch (m_grab) {
    case Grab::None:
        break;
    case Grab::Move:
        m_frame.translate(point - m_last);
        keepInside();
        break;
    case Grab::Left:
        m_frame.setLeft(std::min(point.x(), m_frame.right() - kMinimumSize));
        break;
    case Grab::Right:
        m_frame.setRight(std::max(point.x(), m_frame.left() + kMinimumSize));
        break;
    case Grab::Top:
        m_frame.setTop(std::min(point.y(), m_frame.bottom() - kMinimumSize));
        break;
    case Grab::Bottom:
        m_frame.setBottom(std::max(point.y(), m_frame.top() + kMinimumSize));
        break;
    }
    m_last = point;
}

void Spotlight::release()
{
    m_grab = Grab::None;
}

bool Spotlight::dragging() const noexcept
{
    return m_grab != Grab::None;
}

} // namespace actcraft::features
