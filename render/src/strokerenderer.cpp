#include "nodes.h"

#include <actcraft/render/painternode.h>
#include <actcraft/render/strokegeometry.h>
#include <actcraft/render/strokerenderer.h>

#include <algorithm>
#include <numbers>

namespace actcraft::render {

namespace {

class StrokePainterNode final : public PainterNode
{
public:
    StrokePainterNode(QQuickWindow* window, const core::Stroke& stroke)
        : PainterNode(window), m_stroke(stroke)
    {
        const core::RectF r = strokeBounds(stroke.points, stroke.baseWidth);
        // strokeBounds ignore les coins des bouts carrés (jusqu'à (√2 − 1) × demi-largeur en diagonale) ;
        // le moteur logiciel découpe au rect() : on ajoute cette marge, plus 1 px d'anticrénelage.
        double half = 0;
        for (const auto& p : stroke.points) {
            half = std::max(half, core::strokeWidthAt(stroke.baseWidth, p.pressure) / 2);
        }
        const double margin = half * (std::numbers::sqrt2 - 1.0) + 1.0;
        setRect(QRectF(r.x - margin, r.y - margin, r.width + 2 * margin, r.height + 2 * margin));
    }

protected:
    void paint(QPainter& painter) override
    {
        paintStrokeSegments(painter, m_stroke.points, m_stroke.baseWidth, toQColor(m_stroke.color));
    }

private:
    core::Stroke m_stroke;
};

} // namespace

QSGNode* StrokeRenderer::createNode(const core::PageObject& object, std::size_t,
                                    const RenderContext& context) const
{
    const auto* stroke = std::get_if<core::Stroke>(&object);
    if (!stroke || stroke->points.size() < 2) {
        return nullptr;
    }
    if (context.software) {
        return new StrokePainterNode(context.window, *stroke);
    }
    std::vector<Vertex2D> vertices;
    appendStrokeTriangles(stroke->points, stroke->baseWidth, vertices);
    return makeTrianglesNode(vertices, toQColor(stroke->color));
}

} // namespace actcraft::render
