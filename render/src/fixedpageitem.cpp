#include "objectnodes.h"

#include <actcraft/core/edits.h>
#include <actcraft/core/generator.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/imagesynth.h>

#include <QDebug>
#include <QQuickWindow>
#include <QSGRectangleNode>
#include <QSGRendererInterface>
#include <QSGTransformNode>

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace actcraft::render {

namespace {
// Emplacements du fond peint et de la grille parmi les nœuds rastérisés (les objets y sont par leur indice).
constexpr std::size_t kBackgroundSlot = std::numeric_limits<std::size_t>::max();
constexpr std::size_t kGridSlot = kBackgroundSlot - 1;

// Ce qu'ImageStore::generate reçoit de la page : image du fond, images (synthétiques par synthesisKey,
// ressources par clé) et vignettes des médias, sans leurs indices : le magasin ne range rien par indice
// (L3-C13), un objet retiré avant une image ne change pas ce qu'il décode. Deux pages de même empreinte se
// décodent pareil.
std::string decodeInputs(const core::Page& page)
{
    std::string out = page.background.image;
    for (const core::PageObject& object : page.objects) {
        if (const auto* im = std::get_if<core::ImageObject>(&object)) {
            out += '|' + synthesisKey(*im) + ':' + im->resource;
        } else if (const auto* media = std::get_if<core::Media>(&object)) {
            out += "|m:" + media->placeholder;
        }
    }
    return out;
}

// Objet dont l'image vient du magasin (image, vignette de média) : l'ajouter ou le modifier demande un
// décodage.
bool usesImages(const core::PageObject& object)
{
    return std::get_if<core::ImageObject>(&object) || std::get_if<core::Media>(&object);
}

// Lot 10 : l'objet peut attendre au-dessus du cache (mis en attente par insert, gardé en attente par
// invalidate). Ni image (la couche d'attente n'a pas de magasin), ni encre révélatrice (P8 : elle rend
// transparents les objets sous elle, redessin immédiat), ni page à grille dessinée au-dessus des objets.
// Limite de DA11 (écart 5, H10-8) : sinon, redessin immédiat (reconstruction).
bool canWait(const core::Page& page, const core::PageObject& object)
{
    const auto* stroke = std::get_if<core::Stroke>(&object);
    const bool revealer = stroke && stroke->kind == core::StrokeKind::Revealer;
    return !usesImages(object) && !revealer && !page.grid.drawnAbove;
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

void FixedPageItem::restart(Decode decode, Scope scope)
{
    const bool redecode = decode == Decode::Force;
    const bool samePage = scope == Scope::SamePage;
    // Lot 10 : la reconstruction met tout dans la scène ; ce qui attendait est versé (contentChanged
    // ci-dessous fait redessiner le cache).
    const bool hadPending = !m_pendingIds.empty();
    m_pendingIds.clear();
    if (hadPending) {
        // La couche d'attente garde ses nœuds tant que les identifiants concordent ; un autre document
        // (setSource, setPage) réattribue les mêmes : la révision les lui fait refaire.
        ++m_pendingRevision;
    }
    m_rebuild = true;
    m_builtCount = 0; // les nœuds affichés ne suivent plus cette page : aucun ajout « en fin de scène »
    // Redécoder seulement si le magasin ne couvre plus la page (annuler un trait ne redécode rien). Pendant
    // le décodage de la page affichée, covers() est faux : un trait (même page) ne le relance pas, sinon des
    // traits plus rapides que lui repousseraient le prêt sans fin et l'ancienne page resterait à l'écran.
    // Mais seulement si ses images n'ont pas changé (decodeInputs) ; une suppression avant elles ne compte
    // pas : le magasin les range par synthesisKey (L3-C13).
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
    if (hadPending) {
        emit pendingChanged();
    }
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
    restart(Decode::Force, Scope::OtherPage);
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
    restart(Decode::IfUncovered, Scope::OtherPage);
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
    const bool imageChanged = usesImages(object);
    const bool atEnd = index + 1 == shown.objects.size();
    // Calculé avant tout emit : un slot de objectCountChanged pourrait modifier la page.
    const bool isStroke = std::get_if<core::Stroke>(&object) != nullptr;
    const auto announceAppended = [this, isStroke] {
        emit objectCountChanged();
        if (isStroke) {
            emit strokeAppended();
        }
    };
    if (atEnd && m_deferred && m_ready && canWait(shown, object)) {
        // Lot 10 : il attend au-dessus du cache ; ni nœud dans la scène, ni redessin du cache. Limite de DA11
        // (écart 5, H10-8) : hors fin de page, ou objet qui ne peut pas attendre → redessin immédiat
        // ci-dessous.
        m_pendingIds.push_back(id);
        announceAppended();
        emit pendingChanged();
        return;
    }
    if (atEnd && index >= m_builtCount && !imageChanged && m_pendingIds.empty()) {
        // Fin de page (le stylo, hors différé) : le prochain updatePaintNode n'ajoute que ce nœud. Avec des
        // objets en attente, ce nœud passerait devant eux dans la scène : reconstruction ci-dessous.
        announceAppended();
        emit contentChanged();
        update();
        return;
    }
    // image ajoutée pendant un décodage : il ne la couvre pas
    restart(imageChanged && !m_ready ? Decode::Force : Decode::IfUncovered, Scope::SamePage);
}

// Hors objets en attente : reconstruction complète (lot 2).
void FixedPageItem::remove(core::ObjectId id, const core::RectF&)
{
    const auto it = std::find(m_pendingIds.begin(), m_pendingIds.end(), id);
    if (it != m_pendingIds.end()) {
        // Retiré avant d'être versé (annulation d'un trait récent) : le cache ne l'a jamais contenu.
        m_pendingIds.erase(it);
        if (pendingAreTheLastObjects()) {
            emit objectCountChanged();
            emit pendingChanged();
            return;
        }
        // La page a changé ailleurs aussi : ce qui attend n'est plus sa queue (la couche lit par indice).
    }
    restart(Decode::IfUncovered, Scope::SamePage);
}

void FixedPageItem::invalidate(core::ObjectId id, const core::CacheArea&)
{
    const core::Page& shown = page();
    const std::size_t index = core::indexOf(shown, id);
    const bool imageChanged = index < shown.objects.size() && usesImages(shown.objects[index]);
    const bool pending =
        id != 0 && std::find(m_pendingIds.begin(), m_pendingIds.end(), id) != m_pendingIds.end();
    // index est valide ici : un objet en attente en queue de page y est.
    if (pending && pendingAreTheLastObjects() && canWait(shown, shown.objects[index])) {
        ++m_pendingRevision; // modifié en place pendant qu'il attend : la couche le refait
        emit pendingChanged();
        return;
    }
    // Hors de l'attente, objet qui ne peut plus attendre (canWait : image, encre révélatrice, grille
    // au-dessus), ou objet en attente réordonné (annoncé en ObjectChanged, undo.h n'a pas de type
    // « réordonné ») : ce qui attend n'est plus la queue de la page ; reconstruction complète, qui verse
    // tout, comme insert.
    restart(imageChanged && !m_ready ? Decode::Force : Decode::IfUncovered, Scope::SamePage);
}

void FixedPageItem::setDeferredFolding(bool on)
{
    if (!on) {
        foldPending();
    }
    m_deferred = on;
}

bool FixedPageItem::deferredFolding() const noexcept
{
    return m_deferred;
}

void FixedPageItem::foldPending()
{
    if (m_pendingIds.empty()) {
        return;
    }
    m_pendingIds.clear(); // la prochaine synchronisation ajoute leurs nœuds à la scène (m_builtCount suit)
    emit pendingChanged();
    update();
}

int FixedPageItem::pendingCount() const noexcept
{
    return static_cast<int>(m_pendingIds.size());
}

const std::vector<core::ObjectId>& FixedPageItem::pendingIds() const noexcept
{
    return m_pendingIds;
}

std::uint64_t FixedPageItem::pendingRevision() const noexcept
{
    return m_pendingRevision;
}

std::size_t FixedPageItem::cachedObjectCount() const noexcept
{
    const std::size_t count = page().objects.size();
    return count >= m_pendingIds.size() ? count - m_pendingIds.size() : 0;
}

bool FixedPageItem::forceRasterized() const noexcept
{
    return m_forceRasterized;
}

const ImageStore& FixedPageItem::images() const noexcept
{
    return m_images;
}

bool FixedPageItem::pendingAreTheLastObjects() const noexcept
{
    const core::Page& shown = page();
    if (m_pendingIds.size() > shown.objects.size()) {
        return false;
    }
    const std::size_t first = shown.objects.size() - m_pendingIds.size();
    for (std::size_t k = 0; k < m_pendingIds.size(); ++k) {
        if (shown.objects[first + k].common.id != m_pendingIds[k]) {
            return false;
        }
    }
    return true;
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
    if (!m_pendingIds.empty()) { // la couche d'attente refait ses nœuds à la nouvelle échelle
        ++m_pendingRevision;
        emit pendingChanged();
    }
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
    const NodeContexts contexts =
        nodeContexts(window(), software, &m_images, m_rasterScale, m_forceRasterized);
    const RenderContext& painted = contexts.painted;
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
        // peint. Lot 10 : la scène s'arrête aux objets en attente (dessinés par la couche d'attente) ; elle
        // les ajoute quand ils sont versés.
        const std::size_t limit = cachedObjectCount();
        for (std::size_t i = m_builtCount; i < limit; ++i) {
            const ObjectNode made = createObjectNode(shown.objects[i], i, m_registry, m_painted, contexts);
            if (!made.root) {
                continue;
            }
            if (made.viaPainter && rasterized) {
                // feuille : les nœuds de transformation et d'opacité restent en place
                addRasterSlot(made.leaf, i);
            }
            if (m_gridDrawnAbove) {
                root->insertChildNodeBefore(made.root, m_gridDrawnAbove);
            } else {
                root->appendChildNode(made.root);
            }
        }
        m_builtCount = limit;
    }
    m_rasterNodes.store(static_cast<int>(m_rasterSlots.size()));
    return root;
}

} // namespace actcraft::render
