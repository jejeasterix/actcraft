#include <actcraft/core/edits.h>
#include <actcraft/render/imagestore.h>
#include <actcraft/render/paintedrenderer.h>
#include <actcraft/render/painternode.h>

#include <QPainter>
#include <QQuickWindow>
#include <QSGImageNode>
#include <QSGRectangleNode>
#include <QSGTexture>

#include <algorithm>
#include <cmath>
#include <utility>

namespace actcraft::render {

namespace {

// Images dont l'objet a besoin, copiées (partage implicite de QImage) : le nœud ne lit plus le magasin
// ensuite.
DecodedImages imagesOf(const core::PageObject& object, const ImageStore* store)
{
    DecodedImages out;
    if (!store) {
        return out;
    }
    if (const auto* m = std::get_if<core::Media>(&object); m && !m->placeholder.empty()) {
        out.insert(m->placeholder, store->resourceImage(m->placeholder));
    }
    if (const auto* im = std::get_if<core::ImageObject>(&object); im && !im->resource.empty()) {
        out.insert(im->resource, store->resourceImage(im->resource));
    }
    return out;
}

DecodedImages pictureBackgroundOf(const core::Page& page, const ImageStore* store)
{
    DecodedImages out;
    if (store && !page.background.image.empty()) {
        out.insert(page.background.image, store->resourceImage(page.background.image));
    }
    return out;
}

// Objet en coordonnées de l'objet ; la transformation et la translucidité viennent des nœuds parents.
class PaintedObjectNode final : public PainterNode
{
public:
    PaintedObjectNode(QQuickWindow* window, core::PageObject object, DecodedImages images, const QRectF& rect)
        : PainterNode(window), m_object(std::move(object)), m_images(std::move(images))
    {
        setRect(rect);
    }

protected:
    void paint(QPainter& painter) override { paintObjectLocal(painter, m_object, m_images.lookup()); }

private:
    core::PageObject m_object;
    DecodedImages m_images;
};

class PaintedPageNode final : public PainterNode
{
public:
    enum class Part
    {
        Background,
        Grid,
    };
    PaintedPageNode(QQuickWindow* window, core::Page page, DecodedImages images, Part part)
        : PainterNode(window), m_page(std::move(page)), m_images(std::move(images)), m_part(part)
    {
        setRect(QRectF(0, 0, m_page.width, m_page.height));
    }

protected:
    void paint(QPainter& painter) override
    {
        if (m_part == Part::Background) {
            paintBackground(painter, m_page, m_images.lookup());
        } else {
            paintGrid(painter, m_page, false);
        }
    }

private:
    core::Page m_page; // sans ses objets (copie légère)
    DecodedImages m_images;
    Part m_part;
};

core::Page withoutObjects(const core::Page& page)
{
    core::Page copy;
    copy.width = page.width;
    copy.height = page.height;
    copy.background = page.background;
    copy.grid = page.grid;
    return copy;
}

QSGNode* textureNode(QQuickWindow* window, const QImage& image, const QRectF& rect)
{
    if (image.isNull() || !window) {
        return nullptr;
    }
    QSGImageNode* node = window->createImageNode();
    node->setTexture(window->createTextureFromImage(image));
    node->setOwnsTexture(true);
    node->setFiltering(QSGTexture::Linear);
    node->setRect(rect);
    return node;
}

QImage transparentImage(double width, double height, double scale)
{
    const int w = std::max(1, static_cast<int>(std::ceil(width * scale)));
    const int h = std::max(1, static_cast<int>(std::ceil(height * scale)));
    QImage image(w, h, QImage::Format_ARGB32_Premultiplied);
    image.setDotsPerMeterX(3780); // 96 ppp, comme renderPage
    image.setDotsPerMeterY(3780);
    image.fill(Qt::transparent);
    return image;
}

QImage rasterizePage(const core::Page& page, const ImageLookup& images, double scale, bool grid)
{
    QImage image = transparentImage(page.width, page.height, scale);
    QPainter p(&image);
    p.scale(scale, scale);
    if (grid) {
        paintGrid(p, page, false);
    } else {
        paintBackground(p, page, images);
    }
    p.end();
    return image;
}

double positiveScale(double scale)
{
    return std::isfinite(scale) ? std::max(scale, 0.01) : 1.0;
}

} // namespace

bool usesPaintedRenderer(const core::PageObject& object) noexcept
{
    if (std::get_if<core::Text>(&object) || std::get_if<core::Line>(&object) ||
        std::get_if<core::Media>(&object) || std::get_if<core::Connector>(&object)) {
        return true;
    }
    if (const auto* s = std::get_if<core::Shape>(&object)) {
        return s->kind == core::ShapeKind::Path;
    }
    if (const auto* st = std::get_if<core::Stroke>(&object)) {
        return st->kind != core::StrokeKind::Pen || st->cap != core::PenCap::Square;
    }
    return false;
}

QImage rasterizeLocal(const core::PageObject& object, const ImageLookup& images, double scale,
                      QRectF* localRect)
{
    core::PageObject local = object;
    local.common.transform = core::Transform{};
    const core::RectF b = core::boundsOnPage(local);
    const QRectF rect(b.x, b.y, std::max(b.width, 1.0), std::max(b.height, 1.0));
    if (localRect) {
        *localRect = rect;
    }
    QImage image = transparentImage(rect.width(), rect.height(), scale);
    QPainter p(&image);
    p.scale(scale, scale);
    p.translate(-rect.x(), -rect.y());
    paintObjectLocal(p, local, images);
    p.end();
    return image;
}

QSGNode* PaintedRenderer::createNode(const core::PageObject& object, const RenderContext& context) const
{
    DecodedImages images = imagesOf(object, context.images);
    if (context.software) {
        core::PageObject local = object;
        local.common.transform = core::Transform{};
        const core::RectF b = core::boundsOnPage(local);
        // Marge d'un pixel : l'anticrénelage déborde du rectangle exact.
        const QRectF rect(b.x - 1, b.y - 1, b.width + 2, b.height + 2);
        return new PaintedObjectNode(context.window, std::move(local), std::move(images), rect);
    }
    QRectF rect;
    const QImage image = rasterizeLocal(object, images.lookup(), positiveScale(context.rasterScale), &rect);
    return textureNode(context.window, image, rect);
}

QSGNode* createBackgroundNode(const core::Page& page, const RenderContext& context)
{
    const core::Fill& fill = page.background.fill;
    const QRectF rect(0, 0, page.width, page.height);
    if (page.background.image.empty() &&
        (fill.kind == core::FillKind::Solid || fill.kind == core::FillKind::None)) {
        QSGRectangleNode* node = context.window->createRectangleNode();
        node->setRect(rect);
        // Fond « aucun » : la page reste blanche (F-610).
        node->setColor(fill.kind == core::FillKind::Solid ? QColor::fromRgba(fill.color) : QColor(Qt::white));
        return node;
    }
    DecodedImages images = pictureBackgroundOf(page, context.images);
    if (context.software) {
        return new PaintedPageNode(context.window, withoutObjects(page), std::move(images),
                                   PaintedPageNode::Part::Background);
    }
    return textureNode(context.window,
                       rasterizePage(page, images.lookup(), positiveScale(context.rasterScale), false), rect);
}

QSGNode* createGridNode(const core::Page& page, const RenderContext& context)
{
    if (!page.grid.visible || page.grid.levels.empty()) {
        return nullptr;
    }
    if (context.software) {
        return new PaintedPageNode(context.window, withoutObjects(page), DecodedImages{},
                                   PaintedPageNode::Part::Grid);
    }
    return textureNode(context.window,
                       rasterizePage(page, ImageLookup{}, positiveScale(context.rasterScale), true),
                       QRectF(0, 0, page.width, page.height));
}

} // namespace actcraft::render
