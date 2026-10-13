#include "nodes.h"

#include <actcraft/render/painternode.h>
#include <actcraft/render/shaperenderer.h>
#include <actcraft/render/strokegeometry.h>

#include <QPainter>
#include <QPen>
#include <QPolygonF>

namespace actcraft::render {

namespace {

std::vector<core::PointF> outlineOf(const core::Shape& shape)
{
    switch (shape.kind) {
    case core::ShapeKind::Rectangle:
        return rectanglePolygon(shape.bounds);
    case core::ShapeKind::Ellipse:
        return ellipsePolygon(shape.bounds, 48);
    case core::ShapeKind::Polygon:
        return shape.polygon;
    }
    return {};
}

class ShapePainterNode final : public PainterNode
{
public:
    ShapePainterNode(QQuickWindow* window, const core::Shape& shape)
        : PainterNode(window), m_shape(shape), m_polygon(outlineOf(shape))
    {
        const double m = shape.outlineWidth;
        setRect(QRectF(shape.bounds.x - m, shape.bounds.y - m, shape.bounds.width + 2 * m,
                       shape.bounds.height + 2 * m));
    }

protected:
    void paint(QPainter& painter) override
    {
        QPolygonF polygon;
        for (const auto& p : m_polygon) {
            polygon << QPointF(p.x, p.y);
        }
        painter.setOpacity(painter.opacity() * m_shape.opacity);
        painter.setBrush(toQColor(m_shape.fill));
        if (m_shape.outlineWidth > 0) {
            QPen pen(toQColor(m_shape.outline));
            pen.setWidthF(m_shape.outlineWidth);
            pen.setJoinStyle(Qt::MiterJoin);
            painter.setPen(pen);
        } else {
            painter.setPen(Qt::NoPen);
        }
        painter.drawPolygon(polygon);
    }

private:
    core::Shape m_shape;
    std::vector<core::PointF> m_polygon;
};

} // namespace

QSGNode* ShapeRenderer::createNode(const core::PageObject& object, std::size_t,
                                   const RenderContext& context) const
{
    const auto* shape = std::get_if<core::Shape>(&object);
    if (!shape) {
        return nullptr;
    }
    if (context.software) {
        return new ShapePainterNode(context.window, *shape);
    }
    const auto polygon = outlineOf(*shape);
    auto* root = new QSGNode;
    std::vector<Vertex2D> vertices;
    appendConvexFill(polygon, vertices);
    root->appendChildNode(makeTrianglesNode(vertices, toQColor(shape->fill, shape->opacity)));
    if (shape->outlineWidth > 0) {
        vertices.clear();
        appendOutlineTriangles(polygon, shape->outlineWidth, vertices);
        root->appendChildNode(makeTrianglesNode(vertices, toQColor(shape->outline, shape->opacity)));
    }
    return root;
}

} // namespace actcraft::render
