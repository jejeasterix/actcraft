#include "objectnodes.h"

#include <actcraft/render/liftlayeritem.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/render/paintedrenderer.h>

#include <QMatrix4x4>
#include <QPainter>
#include <QQuickWindow>
#include <QSGImageNode>
#include <QSGRendererInterface>
#include <QSGTexture>
#include <QSGTransformNode>
#include <QTransform>

#include <algorithm>
#include <cmath>
#include <utility>
#include <variant>

namespace actcraft::render {

namespace {
QMatrix4x4 matrixOf(const core::Transform& t)
{
    return QMatrix4x4(QTransform(t.m11, t.m12, t.m21, t.m22, t.dx, t.dy));
}

// Objet dont les images se cherchent dans le magasin de la page fixe (ressource, vignette de média) : tout
// objet sauf l'image synthétique (choix de L3-C1, quand le magasin la rangeait par indice de page). Depuis
// L3-C13, il la range par synthesisKey ; la lui passer est hors du périmètre de L3-C13. Sans magasin, le
// peintre la synthétise lui-même.
bool imagesByKey(const core::PageObject& object)
{
    const auto* image = std::get_if<core::ImageObject>(&object);
    return image == nullptr || !image->resource.empty();
}

// F-255 : encre révélatrice soulevée qui découpe (visible, couche du dessus).
bool cutsTopLayer(const core::PageObject& object)
{
    const auto* stroke = std::get_if<core::Stroke>(&object);
    return stroke && stroke->kind == core::StrokeKind::Revealer && object.common.visible &&
           object.common.layer == core::Layer::Top;
}

// Ce qui est sous la couche du dessus : fond, grille du dessous, couches inférieure et intermédiaire.
core::Page beneathTopLayer(const core::Page& page)
{
    core::Page beneath = page;
    beneath.objects.clear();
    for (const core::PageObject& object : page.objects) {
        if (object.common.layer != core::Layer::Top) {
            beneath.objects.push_back(object);
        }
    }
    return beneath;
}

// Rectangle couvert par l'encre, en coordonnées de page (avant la transformation supplémentaire).
QRectF revealerRect(const core::PageObject& object)
{
    const core::Stroke& stroke = std::get<core::Stroke>(object);
    if (stroke.points.empty()) {
        return {};
    }
    double minX = stroke.points.front().x, maxX = minX, minY = stroke.points.front().y, maxY = minY,
           widest = 0;
    for (const core::StrokePoint& point : stroke.points) {
        minX = std::min(minX, point.x);
        maxX = std::max(maxX, point.x);
        minY = std::min(minY, point.y);
        maxY = std::max(maxY, point.y);
        widest = std::max(widest, core::strokeWidthAt(stroke.baseWidth, point.pressure));
    }
    const double margin = widest / 2 + 2;
    const core::Transform& t = object.common.transform;
    return QTransform(t.m11, t.m12, t.m21, t.m22, t.dx, t.dy)
        .mapRect(QRectF(minX, minY, maxX - minX, maxY - minY).adjusted(-margin, -margin, margin, margin));
}

// Fenêtre de l'encre soulevée : le dessous de la page (resté à sa place : la transformation supplémentaire
// est défaite), vu par la forme de l'encre (CompositionMode_DestinationIn), sur le blanc de la page comme
// renderPage. Peinte par le peintre partagé, à l'échelle de la page fixe ; seulement le rectangle de l'encre.
QSGNode* revealWindowNode(QQuickWindow* window, const core::Page& page, const ImageStore* store,
                          const core::PageObject& revealer, const core::Transform& extra, double scale)
{
    const QRectF rect = revealerRect(revealer);
    if (!window || rect.isEmpty()) {
        return nullptr;
    }
    const core::Page beneath = beneathTopLayer(page);
    DecodedImages images;
    if (store) {
        for (const core::PageObject& object : beneath.objects) {
            if (const auto* im = std::get_if<core::ImageObject>(&object); im && !im->resource.empty()) {
                images.insert(im->resource, store->resourceImage(im->resource));
            } else if (const auto* m = std::get_if<core::Media>(&object); m && !m->placeholder.empty()) {
                images.insert(m->placeholder, store->resourceImage(m->placeholder));
            }
        }
        if (!beneath.background.image.empty()) {
            images.insert(beneath.background.image, store->resourceImage(beneath.background.image));
        }
    }
    const double s = boundedRasterScale(rect.width(), rect.height(), scale);
    QImage image(std::max(1, static_cast<int>(std::ceil(rect.width() * s))),
                 std::max(1, static_cast<int>(std::ceil(rect.height() * s))),
                 QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    {
        QPainter p(&image);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.scale(image.width() / rect.width(), image.height() / rect.height());
        p.translate(-rect.x(), -rect.y());
        p.save();
        bool invertible = false;
        const QTransform undo =
            QTransform(extra.m11, extra.m12, extra.m21, extra.m22, extra.dx, extra.dy).inverted(&invertible);
        if (invertible) {
            p.setTransform(undo, true);
        }
        p.fillRect(QRectF(0, 0, beneath.width, beneath.height), Qt::white);
        PaintOptions options;
        options.grid = !beneath.grid.drawnAbove; // la grille du dessus est une surcouche, au-dessus de tout
        paintPage(p, beneath, images.lookup(), options);
        p.restore();
        core::PageObject mask = revealer;
        std::get<core::Stroke>(mask).color = 0xFF000000u; // la couleur du trait ne compte pas
        p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        paintObject(p, mask, ImageLookup{});
    }
    QSGImageNode* node = window->createImageNode();
    node->setTexture(window->createTextureFromImage(image));
    node->setOwnsTexture(true);
    node->setFiltering(QSGTexture::Linear);
    node->setRect(rect);
    return node;
}
} // namespace

LiftLayerItem::LiftLayerItem(QQuickItem* parent)
    : QQuickItem(parent), m_registry(RendererRegistry::withDefaults())
{
    setFlag(ItemHasContents);
}

FixedPageItem* LiftLayerItem::fixedPage() const noexcept
{
    return m_fixedPage.data();
}

void LiftLayerItem::setFixedPage(FixedPageItem* page)
{
    if (page == m_fixedPage) {
        return;
    }
    m_fixedPage = page;
    m_rebuild = true; // autre contexte (échelle de rastérisation, chemin forcé) : les nœuds sont à refaire
    update();
    emit fixedPageChanged();
}

void LiftLayerItem::setObjects(std::vector<core::PageObject> objects)
{
    m_objects = std::move(objects);
    m_extra = core::Transform{};
    m_rebuild = true;
    update();
    emit objectsChanged();
}

void LiftLayerItem::setExtraTransform(const core::Transform& extra)
{
    // Matrice seule : la synchronisation la pose sur la racine ; aucun nœud d'objet n'est refait.
    m_extra = extra;
    update();
}

void LiftLayerItem::clear()
{
    setObjects({});
}

int LiftLayerItem::objectCount() const noexcept
{
    return static_cast<int>(m_objects.size());
}

int LiftLayerItem::builtCount() const noexcept
{
    return m_builtCount.load();
}

// Fil de rendu, fil principal bloqué : les copies, la matrice et le contexte de la page fixe sont lus tels
// que le fil principal les a laissés.
QSGNode* LiftLayerItem::updatePaintNode(QSGNode* old, UpdatePaintNodeData*)
{
    // Racine de transformation dans les deux chemins : sa matrice est la transformation supplémentaire (en
    // logiciel, c'est aussi la racine qu'exige le piège des nœuds ajoutés sous une racine déjà rendue).
    auto* root = old ? static_cast<QSGTransformNode*>(old) : new QSGTransformNode;
    const QMatrix4x4 matrix = matrixOf(m_extra);
    // F-255 : la fenêtre d'une encre soulevée suit le glisser ; elle seule oblige à refaire les nœuds.
    const bool reveals = std::any_of(m_objects.begin(), m_objects.end(), cutsTopLayer);
    if (m_rebuild || !old || (reveals && root->matrix() != matrix)) {
        // Graphe recréé (old nul) : les anciens nœuds sont partis avec lui ; sinon on les retire ici.
        while (QSGNode* child = root->firstChild()) {
            root->removeChildNode(child);
            delete child;
        }
        if (!m_objects.empty()) {
            const bool software =
                window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software;
            // Contexte de la page fixe (échelle de rastérisation, chemin forcé), comme la couche d'attente :
            // mêmes moteurs, même rendu. Magasin de la page fixe (L3-C1) : il range les ressources par clé et
            // ne les oublie qu'à un nouveau décodage, que le soulèvement d'une image à ressource ne lance pas
            // (ImageStore::covers) ; l'image soulevée y est déjà décodée. Ni second décodage, ni copie du
            // magasin. Image synthétique : sans magasin (imagesByKey).
            const FixedPageItem* page = m_fixedPage.data();
            const double scale = page ? page->rasterScale() : 1.0;
            const bool forced = page != nullptr && page->forceRasterized();
            const bool hiddenShown = page != nullptr && page->showHiddenTranslucent(); // mode conception
            const ImageStore* store = page ? &page->images() : nullptr;
            const NodeContexts byKey = nodeContexts(window(), software, store, scale, forced, hiddenShown);
            const NodeContexts unkeyed =
                nodeContexts(window(), software, nullptr, scale, forced, hiddenShown);
            for (std::size_t i = 0; i < m_objects.size(); ++i) {
                if (cutsTopLayer(m_objects[i])) {
                    if (page) {
                        if (QSGNode* node = revealWindowNode(window(), page->page(), store, m_objects[i],
                                                             m_extra, scale)) {
                            root->appendChildNode(node);
                            ++m_builtCount;
                        }
                    }
                    continue;
                }
                const NodeContexts& contexts = imagesByKey(m_objects[i]) ? byKey : unkeyed;
                const ObjectNode made = createObjectNode(m_objects[i], i, m_registry, m_painted, contexts);
                if (made.root) {
                    root->appendChildNode(made.root); // ordre du vecteur, comme la page fixe
                    ++m_builtCount;
                }
            }
        }
        m_rebuild = false;
    }
    if (root->matrix() != matrix) {
        root->setMatrix(matrix);
    }
    return root;
}

} // namespace actcraft::render
