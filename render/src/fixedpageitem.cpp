#include <actcraft/core/generator.h>
#include <actcraft/render/fixedpageitem.h>

#include <QMatrix4x4>
#include <QQuickWindow>
#include <QSGOpacityNode>
#include <QSGRectangleNode>
#include <QSGRendererInterface>
#include <QSGTransformNode>
#include <QTransform>

#include <algorithm>
#include <cmath>
#include <limits>

namespace actcraft::render {

namespace {
void clearChildren(QSGNode* root)
{
    while (QSGNode* child = root->firstChild()) {
        root->removeChildNode(child);
        delete child;
    }
}

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

// Emplacements du fond peint et de la grille parmi les nœuds rastérisés (les objets y sont par leur indice).
constexpr std::size_t kBackgroundSlot = std::numeric_limits<std::size_t>::max();
constexpr std::size_t kGridSlot = kBackgroundSlot - 1;
} // namespace

FixedPageItem::FixedPageItem(QQuickItem* parent)
    : QQuickItem(parent), m_registry(RendererRegistry::withDefaults())
{
    setFlag(ItemHasContents);
    connect(&m_images, &ImageStore::ready, this, &FixedPageItem::onImagesReady);
}

void FixedPageItem::loadGeneratedPage()
{
    setPage(core::generatePage(core::kDefaultSeed));
}

void FixedPageItem::setPage(core::Page page)
{
    m_page = std::move(page);
    m_rebuild = true;
    if (m_ready) {
        m_ready = false;
        emit readyChanged();
    }
    m_images.generate(m_page);
    emit objectCountChanged();
    update();
}

const core::Page& FixedPageItem::page() const noexcept
{
    return m_page;
}

void FixedPageItem::appendStroke(core::Stroke stroke)
{
    m_page.objects.emplace_back(std::move(stroke));
    emit objectCountChanged();
    emit strokeAppended();
    update();
}

bool FixedPageItem::isReady() const noexcept
{
    return m_ready;
}

int FixedPageItem::objectCount() const noexcept
{
    return static_cast<int>(m_page.objects.size());
}

void FixedPageItem::setRasterScale(double scale)
{
    if (!(scale > 0) || !std::isfinite(scale) || qFuzzyCompare(scale, m_rasterScale)) {
        return;
    }
    m_rasterScale = scale;
    // Seuls les nœuds rastérisés suivent l'échelle, refaits en place : aucun en rendu logiciel ni sur une
    // page sans fond peint, grille ou objet peint (page du banc) ; les nœuds du prototype ne bougent jamais.
    if (m_rasterNodes.load() > 0) {
        m_rescale = true;
        update();
    }
}

double FixedPageItem::rasterScale() const noexcept
{
    return m_rasterScale;
}

int FixedPageItem::rebuildCount() const noexcept
{
    return m_rebuildCount.load();
}

void FixedPageItem::setForceRasterized(bool on)
{
    m_forceRasterized = on;
    m_rebuild = true;
    update();
}

int FixedPageItem::rasterizationCount() const noexcept
{
    return m_rasterizationCount.load();
}

void FixedPageItem::addRasterSlot(QSGNode* node, std::size_t object)
{
    m_rasterSlots.push_back(RasterSlot{node, object});
    ++m_rasterizationCount;
}

void FixedPageItem::rescaleRasterNodes(const core::Page& page, const RenderContext& painted)
{
    for (RasterSlot& slot : m_rasterSlots) {
        QSGNode* fresh = slot.object == kBackgroundSlot ? createBackgroundNode(page, painted)
                         : slot.object == kGridSlot
                             ? createGridNode(page, painted)
                             : m_painted.createNode(page.objects[slot.object], painted);
        if (!fresh) {
            continue;
        }
        QSGNode* parent = slot.node->parent();
        parent->insertChildNodeBefore(fresh, slot.node);
        parent->removeChildNode(slot.node);
        if (m_gridDrawnAbove == slot.node) {
            m_gridDrawnAbove = fresh;
        }
        delete slot.node;
        slot.node = fresh;
        ++m_rasterizationCount;
    }
}

void FixedPageItem::onImagesReady()
{
    m_rebuild = true;
    m_ready = true;
    emit readyChanged();
    update();
}

void FixedPageItem::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    m_rebuild = true;
    update();
}

QSGNode* FixedPageItem::updatePaintNode(QSGNode* old, UpdatePaintNodeData*)
{
    const bool software = window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software;
    // Logiciel : racine de transformation (identité). Sous une racine QSGNode nue déjà rendue, le moteur
    // logiciel de Qt 6.12 peint les nœuds ajoutés sans la transformation de l'élément (voir InkItem).
    QSGNode* root = old ? old : (software ? new QSGTransformNode : new QSGNode);
    RenderContext context;
    context.window = window();
    context.software = software;
    context.images = &m_images;
    context.rasterScale = m_rasterScale;
    // Moteur peint, fond peint et grille : vectoriels en rendu logiciel, textures en RHI (décision 37).
    RenderContext painted = context;
    painted.software = software && !m_forceRasterized;
    const bool rasterized = !painted.software;
    const core::Page& shown = m_page;
    if (m_rebuild || !old) {
        ++m_rebuildCount;
        clearChildren(root);
        m_gridDrawnAbove = nullptr;
        m_rasterSlots.clear();
        if (m_ready) {
            QSGNode* background = createBackgroundNode(shown, painted);
            root->appendChildNode(background);
            if (rasterized && backgroundIsPainted(shown)) {
                addRasterSlot(background, kBackgroundSlot);
            }
            if (QSGNode* grid = createGridNode(shown, painted)) {
                if (shown.grid.drawnAbove) {
                    m_gridDrawnAbove = grid;
                }
                root->appendChildNode(grid);
                if (rasterized) {
                    addRasterSlot(grid, kGridSlot);
                }
            }
        } else {
            QSGRectangleNode* background = window()->createRectangleNode();
            background->setRect(boundingRect());
            background->setColor(Qt::white);
            root->appendChildNode(background); // tant que les images manquent, on ne dessine que le fond
        }
        m_builtCount = 0;
        m_rebuild = !m_ready;
    } else if (m_rescale) {
        rescaleRasterNodes(shown, painted);
    }
    m_rescale = false;
    if (m_ready) {
        // Ordre du vecteur (couche, profondeur, arbre) ; moteur du prototype s'il sait faire, sinon moteur
        // peint.
        for (std::size_t i = m_builtCount; i < shown.objects.size(); ++i) {
            const core::PageObject& object = shown.objects[i];
            if (!object.common.visible || std::get_if<core::Group>(&object)) {
                continue;
            }
            QSGNode* node = nullptr;
            bool viaPainter = usesPaintedRenderer(object);
            if (viaPainter) {
                node = m_painted.createNode(object, painted);
            } else if (const ObjectRenderer* renderer = m_registry.find(core::kindOf(object))) {
                node = renderer->createNode(object, i, context);
                if (!node && std::get_if<core::ImageObject>(&object)) {
                    // Image illisible : le peintre pose son repère, comme la miniature.
                    node = m_painted.createNode(object, painted);
                    viaPainter = true;
                }
            }
            if (!node) {
                continue;
            }
            if (viaPainter && rasterized) {
                addRasterSlot(node, i); // feuille : les nœuds de transformation et d'opacité restent en place
            }
            node = wrapped(node, object.common);
            if (m_gridDrawnAbove) {
                root->insertChildNodeBefore(node, m_gridDrawnAbove);
            } else {
                root->appendChildNode(node);
            }
        }
        m_builtCount = shown.objects.size();
    }
    m_rasterNodes.store(static_cast<int>(m_rasterSlots.size()));
    return root;
}

} // namespace actcraft::render
