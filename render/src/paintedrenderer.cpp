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

// Image transparente qui recouvre exactement `rect` à l'échelle bornée.
QImage rasterTarget(const QRectF& rect, double scale)
{
    return transparentImage(rect.width(), rect.height(),
                            boundedRasterScale(rect.width(), rect.height(), scale));
}

// Le peintre dessine ensuite en coordonnées de `rect`.
void mapOnto(QPainter& painter, const QImage& image, const QRectF& rect)
{
    painter.scale(image.width() / rect.width(), image.height() / rect.height());
    painter.translate(-rect.x(), -rect.y());
}

QRectF pageRect(const core::Page& page)
{
    return QRectF(0, 0, std::max(page.width, 1.0), std::max(page.height, 1.0));
}

QImage rasterizePage(const core::Page& page, const ImageLookup& images, double scale, bool grid)
{
    const QRectF rect = pageRect(page);
    QImage image = rasterTarget(rect, scale);
    QPainter p(&image);
    mapOnto(p, image, rect);
    if (grid) {
        paintGrid(p, page, false);
    } else {
        paintBackground(p, page, images);
    }
    p.end();
    return image;
}

void unitePoint(QRectF& rect, double x, double y, double reach)
{
    rect = rect.united(QRectF(x - reach, y - reach, 2 * reach, 2 * reach));
}

// Rectangle couvert par le dessin en coordonnées de l'objet : boîte du modèle, élargie de la largeur
// réellement dessinée autour de chaque sommet (le peintre trace les traits autres que le stylo carré à
// largeur constante, pas à celle de la pression). Une largeur entière couvre le bout rond (1/2), le bout
// carré en diagonale (√2/2) et l'onglet (limite 2 de Qt : 1/2 × 2).
QRectF drawnRect(const core::PageObject& local)
{
    const core::RectF b = core::boundsOnPage(local);
    QRectF rect(b.x, b.y, b.width, b.height);
    if (const auto* st = std::get_if<core::Stroke>(&local)) {
        double width = st->baseWidth;
        for (const core::StrokePoint& q : st->points) {
            width = std::max(width, core::strokeWidthAt(st->baseWidth, q.pressure));
        }
        for (const core::StrokePoint& q : st->points) {
            unitePoint(rect, q.x, q.y, width);
        }
    } else if (const auto* l = std::get_if<core::Line>(&local)) {
        for (const core::PointF& q : l->points) {
            unitePoint(rect, q.x, q.y, std::max(l->pen.width, 1.0));
        }
    } else if (const auto* c = std::get_if<core::Connector>(&local)) {
        unitePoint(rect, c->firstEnd.x, c->firstEnd.y, std::max(c->pen.width, 1.0));
        unitePoint(rect, c->secondEnd.x, c->secondEnd.y, std::max(c->pen.width, 1.0));
    } else if (const auto* sh = std::get_if<core::Shape>(&local); sh && sh->kind == core::ShapeKind::Path) {
        const double reach = sh->pen.style == core::PenStyle::None ? 0.0 : std::max(sh->pen.width, 1.0);
        for (const auto& path : sh->paths) {
            for (const core::PointF& q : path) {
                unitePoint(rect, q.x, q.y, reach);
            }
        }
    }
    return rect;
}

QColor overWhite(core::Argb argb)
{
    const QColor c = QColor::fromRgba(argb);
    const double a = c.alphaF();
    const auto blend = [a](int channel) { return qRound(channel * a + 255.0 * (1.0 - a)); };
    return QColor(blend(c.red()), blend(c.green()), blend(c.blue()));
}

} // namespace

double boundedRasterScale(double width, double height, double scale) noexcept
{
    double s = std::isfinite(scale) ? std::max(scale, 0.01) : 1.0;
    const double w = std::max(width, 1.0);
    const double h = std::max(height, 1.0);
    if (!std::isfinite(w) || !std::isfinite(h)) {
        return s;
    }
    s = std::min({s, std::sqrt(kMaxRasterPixels / (w * h)), kMaxRasterSide / w, kMaxRasterSide / h});
    const auto fits = [w, h](double k) {
        const double cw = std::ceil(w * k);
        const double ch = std::ceil(h * k);
        return cw * ch <= kMaxRasterPixels && cw <= kMaxRasterSide && ch <= kMaxRasterSide;
    };
    while (s > 1e-9 && !fits(s)) {
        s *= 0.999;
    }
    return s;
}

bool usesPaintedRenderer(const core::PageObject& object) noexcept
{
    if (const auto* u = std::get_if<core::UnknownObject>(&object)) {
        return u->hasBounds; // repère peint ; sans boîte, aucun moteur : rien à dessiner
    }
    if (std::get_if<core::Text>(&object) || std::get_if<core::Line>(&object) ||
        std::get_if<core::Media>(&object) || std::get_if<core::Connector>(&object) ||
        std::get_if<core::Equation>(&object)) {
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
    QRectF rect = drawnRect(local);
    rect.setSize(QSizeF(std::max(rect.width(), 1.0), std::max(rect.height(), 1.0)));
    if (localRect) {
        *localRect = rect;
    }
    QImage image = rasterTarget(rect, scale);
    QPainter p(&image);
    mapOnto(p, image, rect);
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
        // Marge d'un pixel : l'anticrénelage déborde du rectangle exact.
        const QRectF rect = drawnRect(local).adjusted(-1, -1, 1, 1);
        return new PaintedObjectNode(context.window, std::move(local), std::move(images), rect);
    }
    QRectF rect;
    const QImage image = rasterizeLocal(object, images.lookup(), context.rasterScale, &rect);
    return textureNode(context.window, image, rect);
}

bool backgroundIsPainted(const core::Page& page) noexcept
{
    const core::FillKind kind = page.background.fill.kind;
    return !page.background.image.empty() || (kind != core::FillKind::Solid && kind != core::FillKind::None);
}

QSGNode* createBackgroundNode(const core::Page& page, const RenderContext& context)
{
    const core::Fill& fill = page.background.fill;
    // Blanc de la page sous le fond, comme renderPage : ce que le fond ne couvre pas reste blanc, quelle que
    // soit la couleur derrière l'élément.
    QSGRectangleNode* white = context.window->createRectangleNode();
    white->setRect(QRectF(0, 0, page.width, page.height));
    if (!backgroundIsPainted(page)) {
        white->setColor(fill.kind == core::FillKind::Solid ? overWhite(fill.color) : QColor(Qt::white));
        return white;
    }
    white->setColor(Qt::white);
    DecodedImages images = pictureBackgroundOf(page, context.images);
    QSGNode* painted = nullptr;
    if (context.software) {
        painted = new PaintedPageNode(context.window, withoutObjects(page), std::move(images),
                                      PaintedPageNode::Part::Background);
    } else {
        painted = textureNode(
            context.window, rasterizePage(page, images.lookup(), context.rasterScale, false), pageRect(page));
    }
    if (painted) {
        white->appendChildNode(painted);
    }
    return white;
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
    return textureNode(context.window, rasterizePage(page, ImageLookup{}, context.rasterScale, true),
                       pageRect(page));
}

} // namespace actcraft::render
