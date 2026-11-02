#include "objectnodes.h"

#include <QMatrix4x4>
#include <QSGOpacityNode>
#include <QSGTransformNode>
#include <QTransform>

namespace actcraft::render {

NodeContexts nodeContexts(QQuickWindow* window, bool software, const ImageStore* images, double rasterScale,
                          bool forceRasterized)
{
    NodeContexts out;
    out.context.window = window;
    out.context.software = software;
    out.context.images = images;
    out.context.rasterScale = rasterScale;
    out.painted = out.context;
    out.painted.software = software && !forceRasterized;
    return out;
}

void clearChildren(QSGNode* root)
{
    while (QSGNode* child = root->firstChild()) {
        root->removeChildNode(child);
        delete child;
    }
}

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
                            const NodeContexts& contexts)
{
    ObjectNode out;
    if (!object.common.visible || std::get_if<core::Group>(&object)) {
        return out;
    }
    QSGNode* node = nullptr;
    bool viaPainter = usesPaintedRenderer(object);
    if (viaPainter) {
        node = painter.createNode(object, contexts.painted);
    } else if (const ObjectRenderer* renderer = registry.find(core::kindOf(object))) {
        node = renderer->createNode(object, index, contexts.context);
        if (!node && std::get_if<core::ImageObject>(&object)) {
            node = painter.createNode(object, contexts.painted); // image illisible : repère du peintre
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
