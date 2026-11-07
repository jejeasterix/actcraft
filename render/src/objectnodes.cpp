#include "objectnodes.h"

#include <actcraft/render/imagestore.h>
#include <actcraft/render/pagepainter.h>

#include <QMatrix4x4>
#include <QSGOpacityNode>
#include <QSGTransformNode>
#include <QTransform>

#include <variant>

namespace actcraft::render {

NodeContexts nodeContexts(QQuickWindow* window, bool software, const ImageStore* images, double rasterScale,
                          bool forceRasterized, bool showHidden)
{
    NodeContexts out;
    out.context.window = window;
    out.context.software = software;
    out.context.images = images;
    out.context.rasterScale = rasterScale;
    out.painted = out.context;
    out.painted.software = software && !forceRasterized;
    out.showHidden = showHidden;
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
    // Objet masqué montré par le mode conception : translucide, en plus de sa propre translucidité.
    const double shown = common.visible ? 1.0 : kHiddenShownOpacity;
    if (common.translucency != 0 || !common.visible) {
        auto* opacity = new QSGOpacityNode;
        opacity->setOpacity(shown * (1.0 - common.translucency / 255.0));
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
    if ((!object.common.visible && !contexts.showHidden) || std::get_if<core::Group>(&object)) {
        return out;
    }
    // F-255 : une encre révélatrice ne se dessine jamais comme un trait ; sa découpe est le fait du peintre
    // partagé (page fixe) ou de la fenêtre de la couche soulevée.
    if (const auto* stroke = std::get_if<core::Stroke>(&object);
        stroke && stroke->kind == core::StrokeKind::Revealer) {
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

void addObjectImages(DecodedImages& images, const core::PageObject& object, const ImageStore& store)
{
    if (const auto* im = std::get_if<core::ImageObject>(&object); im && !im->resource.empty()) {
        images.insert(im->resource, store.resourceImage(im->resource));
    } else if (const auto* m = std::get_if<core::Media>(&object); m && !m->placeholder.empty()) {
        images.insert(m->placeholder, store.resourceImage(m->placeholder));
    }
}

} // namespace actcraft::render
