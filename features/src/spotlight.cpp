#include <actcraft/features/spotlight.h>

#include <algorithm>
#include <cmath>
#include <iterator>

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
    const bool alongX = point.x() >= f.left() - kRimMargin && point.x() <= f.right() + kRimMargin;
    const bool alongY = point.y() >= f.top() - kRimMargin && point.y() <= f.bottom() + kRimMargin;
    struct Candidate
    {
        Grab grab;
        double distance;
    };
    const double none = kRimMargin + 1;
    const Candidate candidates[] = {
        {Grab::Left, alongY ? std::abs(point.x() - f.left()) : none},
        {Grab::Right, alongY ? std::abs(point.x() - f.right()) : none},
        {Grab::Top, alongX ? std::abs(point.y() - f.top()) : none},
        {Grab::Bottom, alongX ? std::abs(point.y() - f.bottom()) : none},
    };
    const auto best =
        std::min_element(std::begin(candidates), std::end(candidates),
                         [](const Candidate& a, const Candidate& b) { return a.distance < b.distance; });
    if (best->distance <= kRimMargin) {
        m_grab = best->grab;
    } else {
        // Cache plein comme spot : le pourtour est testé d'abord, l'intérieur ne se saisit qu'ailleurs.
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
