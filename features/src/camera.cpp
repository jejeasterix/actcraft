#include <actcraft/features/camera.h>

#include <QPainter>
#include <QPainterPath>

#include <cmath>

namespace actcraft::features {

void PolygonPicker::click(QPointF point)
{
    if (m_closed)
        return;
    if (m_points.size() >= 3) {
        const QPointF d = point - m_points.first();
        if (std::hypot(d.x(), d.y()) <= kClosePixels) {
            m_closed = true;
            return;
        }
    }
    m_points.append(point);
}

bool PolygonPicker::closed() const noexcept
{
    return m_closed;
}

QPolygonF PolygonPicker::polygon() const
{
    return m_points;
}

void PolygonPicker::clear()
{
    m_points.clear();
    m_closed = false;
}

void FreehandPicker::press(QPointF point)
{
    clear();
    m_points.append(point);
}

void FreehandPicker::move(QPointF point)
{
    if (m_closed || m_points.isEmpty())
        return;
    m_points.append(point);
}

void FreehandPicker::release()
{
    if (m_closed || m_points.size() < 3)
        return;
    m_points.append(m_points.first()); // fermeture par une droite
    m_closed = true;
}

bool FreehandPicker::closed() const noexcept
{
    return m_closed;
}

QPolygonF FreehandPicker::polygon() const
{
    return m_points;
}

void FreehandPicker::clear()
{
    m_points.clear();
    m_closed = false;
}

QImage cropToPolygon(const QImage& screen, const QPolygonF& polygon)
{
    const QRect box = polygon.boundingRect().toAlignedRect() & screen.rect();
    if (box.isEmpty())
        return {};
    QImage out(box.size(), QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);
    QPainter painter(&out);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath path;
    path.addPolygon(polygon.translated(-box.topLeft()));
    path.closeSubpath();
    painter.setClipPath(path);
    painter.drawImage(-box.topLeft(), screen);
    return out;
}

QRectF normalizedRect(QPointF a, QPointF b)
{
    return QRectF(a, b).normalized();
}

QRectF toLogicalPixels(const QRect& physical, qreal devicePixelRatio)
{
    const qreal ratio = devicePixelRatio > 0 ? devicePixelRatio : 1;
    return QRectF(physical.x() / ratio, physical.y() / ratio, physical.width() / ratio,
                  physical.height() / ratio);
}

} // namespace actcraft::features
