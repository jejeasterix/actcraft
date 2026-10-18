#include "objectnodes.h"

#include <QMatrix4x4>
#include <QSGOpacityNode>
#include <QSGTransformNode>
#include <QTransform>

namespace actcraft::render {

namespace {
// Transformation et translucidité de l'objet par des nœuds parents, quel que soit son moteur : aucun moteur
// ne les applique lui-même (sinon double transformation).
QSGNode* wrapped(QSGNode* node, const core::ObjectCommon& common)
{
    if (!common.transform.isIdentity()) {
        const core::Transform& t = common.transform;
        auto* transform = new QSGTransformNode;
        transform->setMatrix(QMatrix4x4(QTransform(t.m11, t.m12, t.m21, t.m22, t.dx, t.dy)));
        transform->appendChildNode(node);
        node = transform;
    }
    if (common.translucency != 0) {
        auto* opacity = new QSGOpacityNode;
        opacity->setOpacity(1.0 - common.translucency / 255.0);
        opacity->appendChildNode(node);
        node = opacity;
    }
    return node;
}
} // namespace

ObjectNode createObjectNode(const core::PageObject& object, std::size_t index,
                            const RendererRegistry& registry, const PaintedRenderer& painter,
                            const RenderContext& context, const RenderContext& painted)
{
    ObjectNode out;
    if (!object.common.visible || std::get_if<core::Group>(&object)) {
        return out;
    }
    QSGNode* node = nullptr;
    bool viaPainter = usesPaintedRenderer(object);
    if (viaPainter) {
        node = painter.createNode(object, painted);
    } else if (const ObjectRenderer* renderer = registry.find(core::kindOf(object))) {
        node = renderer->createNode(object, index, context);
        if (!node && std::get_if<core::ImageObject>(&object)) {
            node = painter.createNode(object, painted); // image illisible : repère du peintre
            viaPainter = true;
        }
    }
    if (!node) {
        return out;
    }
    out.leaf = node;
    out.viaPainter = viaPainter;
    out.root = wrapped(node, object.common);
    return out;
}

} // namespace actcraft::render
