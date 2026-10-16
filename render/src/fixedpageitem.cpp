#include <actcraft/core/edits.h>
#include <actcraft/core/generator.h>
#include <actcraft/render/fixedpageitem.h>

#include <QDebug>
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
#include <string>

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

// Ce qu'ImageStore::generate reçoit de la page : image du fond, images (synthétiques rangées par INDICE
// d'objet, ressources par clé) et vignettes des médias. Deux pages de même empreinte se décodent pareil.
std::string decodeInputs(const core::Page& page)
{
    std::string out = page.background.image;
    for (std::size_t i = 0; i < page.objects.size(); ++i) {
        if (const auto* im = std::get_if<core::ImageObject>(&page.objects[i])) {
            out += '|' + std::to_string(i) + ':' + std::to_string(static_cast<int>(im->pattern)) + ':' +
                   std::to_string(im->colorA) + ':' + std::to_string(im->colorB) + ':' +
                   std::to_string(im->seed) + ':' + std::to_string(im->sourceWidth) + 'x' +
                   std::to_string(im->sourceHeight) + ':' +
                   (im->hasKeyColor ? std::to_string(im->keyColor) : std::string()) + ':' + im->resource;
        } else if (const auto* media = std::get_if<core::Media>(&page.objects[i])) {
            out += '|' + std::to_string(i) + ":m:" + media->placeholder;
        }
    }
    return out;
}
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

const core::Document& FixedPageItem::document() const noexcept
{
    return m_source ? *m_source : m_own;
}

const core::Page& FixedPageItem::page() const noexcept
{
    if (m_source && m_sourcePage < m_source->pageCount()) {
        return m_source->page(m_sourcePage);
    }
    return m_own.page(0);
}

void FixedPageItem::restart(bool redecode, bool samePage)
{
    m_rebuild = true;
    m_builtCount = 0; // les nœuds affichés ne suivent plus cette page : aucun ajout « en fin de scène »
    // Redécoder seulement si le magasin ne couvre plus la page (annuler un trait ne redécode rien). Pendant
    // le décodage de la page affichée, covers() est faux : un trait (même page) ne le relance pas, sinon des
    // traits plus rapides que lui repousseraient le prêt sans fin et l'ancienne page resterait à l'écran.
    // Mais seulement si ses images n'ont pas bougé : une suppression avant elles décale leurs indices, et
    // le décodage en cours poserait l'image d'un objet sur son voisin.
    const bool decodingThisPage = samePage && !m_ready && decodeInputs(page()) == m_decodingInputs;
    if (redecode || (!decodingThisPage && !m_images.covers(page(), &document()))) {
        if (m_ready) {
            m_ready = false;
            emit readyChanged();
        }
        ++m_decodeCount;
        m_decodingInputs = decodeInputs(page());
        m_images.generate(page(), &document());
    }
    emit objectCountChanged();
    emit contentChanged();
    update();
}

void FixedPageItem::setPage(core::Page page)
{
    m_source = nullptr;
    m_sourcePage = 0;
    std::vector<core::Page> pages;
    pages.push_back(std::move(page));
    m_own = core::Document(std::move(pages));
    // Redécodage forcé : nécessaire au relais du lot 1 (la vue ne redessinait que sur readyChanged) ;
    // redondant depuis L2-T15 (la vue écoute contentChanged), gardé : il redécode à chaque setPage (banc et
    // tests).
    restart(true, false);
}

void FixedPageItem::setSource(const core::Document* document, std::size_t pageIndex)
{
    if (document && pageIndex >= document->pageCount()) {
        qWarning().nospace() << "FixedPageItem::setSource : page " << pageIndex
                             << " absente (le document en a " << document->pageCount()
                             << ") ; page interne vide";
        document = nullptr;
    }
    m_source = document;
    m_sourcePage = document ? pageIndex : 0;
    m_own = core::Document(); // l'ancienne page interne ne revient jamais en repli
    restart(false, false);
}

const core::Document* FixedPageItem::sourceDocument() const noexcept
{
    return m_source;
}

std::size_t FixedPageItem::sourcePage() const noexcept
{
    return m_sourcePage;
}

void FixedPageItem::appendStroke(core::Stroke stroke)
{
    if (m_source) {
        qWarning(
            "FixedPageItem::appendStroke : la page affichée appartient à un document ; passer par sa pile");
        return;
    }
    core::PageObject object = std::move(stroke);
    object.common.id = m_own.allocateId();
    object.common.layer = core::Layer::Top;
    const core::ObjectId id = object.common.id;
    core::insertAtTopOfLayer(m_own.page(0), std::move(object));
    insert(id);
}

void FixedPageItem::insert(core::ObjectId id)
{
    const core::Page& shown = page();
    const std::size_t index = core::indexOf(shown, id);
    if (index == shown.objects.size()) {
        return;
    }
    const core::PageObject& object = shown.objects[index];
    const bool needsImages = std::get_if<core::ImageObject>(&object) || std::get_if<core::Media>(&object);
    if (index + 1 == shown.objects.size() && index >= m_builtCount && !needsImages) {
        // Fin de page (le stylo) : le prochain updatePaintNode n'ajoute que ce nœud.
        emit objectCountChanged();
        if (std::get_if<core::Stroke>(&object)) {
            emit strokeAppended();
        }
        emit contentChanged();
        update();
        return;
    }
    restart(needsImages && !m_ready, true); // image ajoutée pendant un décodage : il ne la couvre pas
}

// Lot 2 : reconstruction complète ; le lot « cache incrémental » visera le nœud de `id` et la zone.
void FixedPageItem::remove(core::ObjectId, const core::RectF&)
{
    restart(false, true);
}

void FixedPageItem::invalidate(core::ObjectId id, const core::CacheArea&)
{
    const core::Page& shown = page();
    const std::size_t index = core::indexOf(shown, id);
    const bool image =
        index < shown.objects.size() && (std::get_if<core::ImageObject>(&shown.objects[index]) ||
                                         std::get_if<core::Media>(&shown.objects[index]));
    restart(image && !m_ready, true);
}

bool FixedPageItem::isReady() const noexcept
{
    return m_ready;
}

int FixedPageItem::objectCount() const noexcept
{
    return static_cast<int>(page().objects.size());
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

int FixedPageItem::decodeCount() const noexcept
{
    return m_decodeCount;
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
    const core::Page& shown = page();
    if (m_rebuild || !old) {
        // Autre page en décodage (page suivante, setPage) : la page précédente reste à l'écran jusqu'au prêt,
        // au lieu d'un fond blanc nu (éclair blanc, revue L1-T9, L2-T23). Ni reconstruction ni changement
        // d'échelle sur ces nœuds : leurs indices visent l'ancienne page ; onImagesReady relance la
        // reconstruction. Sans page déjà montrée (premier dessin), le fond blanc d'attente est dessiné.
        if (!m_ready && old && m_showsPage) {
            if (boundingRect() != m_shownRect) {
                // Page suivante d'une autre taille (l'élément prend sa taille) : fond de page blanc à la
                // nouvelle taille sous l'ancienne page, jamais un trou à la couleur de la fenêtre. Plus
                // petite, l'ancienne page dépasse jusqu'au prêt.
                if (!m_underlay) {
                    m_underlay = window()->createRectangleNode();
                    m_underlay->setColor(Qt::white);
                    root->prependChildNode(m_underlay);
                }
                m_underlay->setRect(boundingRect());
            } else if (m_underlay) { // revenue à la taille de la page montrée : le fond ne déborde plus
                root->removeChildNode(m_underlay);
                delete m_underlay;
                m_underlay = nullptr;
            }
            return root;
        }
        ++m_rebuildCount;
        clearChildren(root);
        m_underlay = nullptr;
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
        m_showsPage = m_ready;
        if (m_ready) {
            m_shownRect = boundingRect();
        }
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
