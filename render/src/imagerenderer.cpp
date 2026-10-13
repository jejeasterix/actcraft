#include <actcraft/render/imagerenderer.h>
#include <actcraft/render/imagestore.h>

#include <QQuickWindow>
#include <QSGImageNode>
#include <QSGTexture>

namespace actcraft::render {

QSGNode* ImageRenderer::createNode(const core::PageObject& object, std::size_t objectIndex,
                                   const RenderContext& context) const
{
    const auto* image = std::get_if<core::ImageObject>(&object);
    if (!image || !context.images || !context.window) {
        return nullptr;
    }
    const QImage pixels = context.images->image(objectIndex);
    if (pixels.isNull()) {
        return nullptr;
    }
    QSGImageNode* node = context.window->createImageNode();
    node->setTexture(context.window->createTextureFromImage(pixels));
    node->setOwnsTexture(true);
    node->setFiltering(QSGTexture::Linear);
    node->setRect(QRectF(image->bounds.x, image->bounds.y, image->bounds.width, image->bounds.height));
    return node;
}

} // namespace actcraft::render
