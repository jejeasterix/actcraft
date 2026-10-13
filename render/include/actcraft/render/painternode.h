#pragma once

#include <actcraft/core/model.h>

#include <QColor>
#include <QRectF>
#include <QSGRenderNode>

#include <span>

class QPainter;
class QQuickWindow;

namespace actcraft::render {

// Nœud du rendu logiciel : un QSGRenderNode peint au QPainter (décision 6), à rectangle borné pour garder les
// mises à jour partielles. Les sous-classes ne font que peindre en coordonnées de l'élément.
class PainterNode : public QSGRenderNode
{
public:
    explicit PainterNode(QQuickWindow* window);

    void render(const RenderState* state) override;
    StateFlags changedStates() const override;
    RenderingFlags flags() const override;
    QRectF rect() const override;
    void setRect(const QRectF& rect);

protected:
    virtual void paint(QPainter& painter) = 0;

private:
    QQuickWindow* m_window;
    QRectF m_rect;
};

// Segments d'un trait au QPainter : même géométrie que appendStrokeTriangles (bouts carrés).
void paintStrokeSegments(QPainter& painter, std::span<const core::StrokePoint> points, double baseWidth,
                         QColor color);

} // namespace actcraft::render
