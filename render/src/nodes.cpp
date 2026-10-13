#include "nodes.h"

#include <actcraft/render/objectrenderer.h>

#include <QSGFlatColorMaterial>

#include <cstring>

namespace actcraft::render {

static_assert(sizeof(Vertex2D) == sizeof(QSGGeometry::Point2D));

QColor toQColor(core::Argb argb, double opacity)
{
    QColor color = QColor::fromRgba(argb);
    color.setAlphaF(static_cast<float>(color.alphaF() * opacity));
    return color;
}

QSGGeometryNode* makeTrianglesNode(const std::vector<Vertex2D>& vertices, QColor color)
{
    auto* geometry =
        new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), static_cast<int>(vertices.size()));
    geometry->setDrawingMode(QSGGeometry::DrawTriangles);
    if (!vertices.empty()) {
        std::memcpy(geometry->vertexData(), vertices.data(), vertices.size() * sizeof(Vertex2D));
    }
    auto* material = new QSGFlatColorMaterial;
    material->setColor(color);
    auto* node = new QSGGeometryNode;
    node->setGeometry(geometry);
    node->setMaterial(material);
    node->setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial);
    return node;
}

} // namespace actcraft::render
