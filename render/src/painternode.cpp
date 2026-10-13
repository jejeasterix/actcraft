#include <actcraft/render/painternode.h>

#include <QPainter>
#include <QPen>
#include <QQuickWindow>
#include <QSGRendererInterface>

namespace actcraft::render {

PainterNode::PainterNode(QQuickWindow* window) : m_window(window) {}

void PainterNode::render(const RenderState* state)
{
    auto* painter = static_cast<QPainter*>(
        m_window->rendererInterface()->getResource(m_window, QSGRendererInterface::PainterResource));
    if (!painter) {
        return;
    }
    painter->save();
    // La région de découpe est en coordonnées du périphérique : la poser avant la transformation.
    if (const QRegion* clip = state->clipRegion(); clip && !clip->isEmpty()) {
        painter->setClipRegion(*clip, Qt::ReplaceClip);
    }
    painter->setTransform(matrix()->toTransform());
    painter->setOpacity(inheritedOpacity());
    painter->setRenderHint(QPainter::Antialiasing, true);
    paint(*painter);
    painter->restore();
}

QSGRenderNode::StateFlags PainterNode::changedStates() const
{
    return {};
}

QSGRenderNode::RenderingFlags PainterNode::flags() const
{
    return BoundedRectRendering;
}

QRectF PainterNode::rect() const
{
    return m_rect;
}

void PainterNode::setRect(const QRectF& rect)
{
    m_rect = rect;
}

void paintStrokeSegments(QPainter& painter, std::span<const core::StrokePoint> points, double baseWidth,
                         QColor color)
{
    QPen pen(color);
    pen.setCapStyle(Qt::SquareCap);
    for (std::size_t i = 0; i + 1 < points.size(); ++i) {
        pen.setWidthF(core::strokeWidthAt(baseWidth, points[i].pressure));
        painter.setPen(pen);
        painter.drawLine(QPointF(points[i].x, points[i].y), QPointF(points[i + 1].x, points[i + 1].y));
    }
}

} // namespace actcraft::render
