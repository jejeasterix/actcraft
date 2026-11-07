#include "objectnodes.h"

#include <actcraft/core/edits.h>
#include <actcraft/core/generator.h>
#include <actcraft/core/pagesettings.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/imagesynth.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/render/painternode.h>

#include <QDebug>
#include <QPainter>
#include <QQuickWindow>
#include <QSGImageNode>
#include <QSGRectangleNode>
#include <QSGRendererInterface>
#include <QSGTexture>
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
constexpr std::size_t kRevealSlot = kBackgroundSlot - 2; // couche du dessus découpée (F-255)

bool isRevealer(const core::PageObject& object)
{
    const auto* stroke = std::get_if<core::Stroke>(&object);
    return stroke && stroke->kind == core::StrokeKind::Revealer;
}

// F-255 : objets de la couche du dessus jusqu'à la dernière encre révélatrice visible, parmi les `limit`
// premiers (ordre du vecteur : couche, profondeur) ; `any` faux : aucune.
struct RevealRange
{
    std::size_t first = 0;
    std::size_t last = 0;
    bool any = false;
};

RevealRange revealRange(const core::Page& page, std::size_t limit)
{
    RevealRange r;
    for (std::size_t i = std::min(limit, page.objects.size()); i-- > 0;) {
        const core::PageObject& o = page.objects[i];
        if (isRevealer(o) && o.common.visible && o.common.layer == core::Layer::Top) {
            r.last = i;
            r.any = true;
            break;
        }
    }
    if (r.any) {
        r.first = r.last;
        while (r.first > 0 && page.objects[r.first - 1].common.layer == core::Layer::Top) {
            --r.first;
        }
    }
    return r;
}

// Logiciel : la part découpée peinte au QPainter par le peintre partagé, à chaque rendu.
class RevealedLayerNode final : public PainterNode
{
public:
    RevealedLayerNode(QQuickWindow* window, core::Page part, DecodedImages images, PaintOptions options)
        : PainterNode(window), m_part(std::move(part)), m_images(std::move(images)), m_options(options)
    {
        setRect(QRectF(0, 0, m_part.width, m_part.height));
    }

protected:
    void paint(QPainter& painter) override { paintPage(painter, m_part, m_images.lookup(), m_options); }

private:
    core::Page m_part;
    DecodedImages m_images;
    PaintOptions m_options;
};

// La découpe n'existe qu'au peintre partagé (paintPage) : la page fixe lui confie la part de la couche du
// dessus qu'une encre révélatrice touche, en un seul nœud (vectoriel en logiciel, texture en RHI), au lieu
// d'un nœud par objet ; le cache la photographie comme le reste.
QSGNode* createRevealedNode(const core::Page& page, const RevealRange& r, const ImageStore& store,
                            const RenderContext& painted, bool showHidden, double scale)
{
    core::Page part;
    part.width = page.width;
    part.height = page.height;
    part.objects.assign(page.objects.begin() + static_cast<std::ptrdiff_t>(r.first),
                        page.objects.begin() + static_cast<std::ptrdiff_t>(r.last + 1));
    DecodedImages images;
    for (const core::PageObject& object : part.objects) {
        if (const auto* im = std::get_if<core::ImageObject>(&object); im && !im->resource.empty()) {
            images.insert(im->resource, store.resourceImage(im->resource));
        } else if (const auto* m = std::get_if<core::Media>(&object); m && !m->placeholder.empty()) {
            images.insert(m->placeholder, store.resourceImage(m->placeholder));
        }
    }
    PaintOptions options;
    options.background = false;
    options.grid = false;
    options.showHiddenTranslucent = showHidden;
    if (painted.software) {
        return new RevealedLayerNode(painted.window, std::move(part), std::move(images), options);
    }
    const QRectF rect(0, 0, std::max(part.width, 1.0), std::max(part.height, 1.0));
    const double s = boundedRasterScale(rect.width(), rect.height(), scale);
    QImage image(std::max(1, static_cast<int>(std::ceil(rect.width() * s))),
                 std::max(1, static_cast<int>(std::ceil(rect.height() * s))),
                 QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    {
        QPainter p(&image);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.scale(image.width() / rect.width(), image.height() / rect.height());
        paintPage(p, part, images.lookup(), options);
    }
    QSGImageNode* node = painted.window->createImageNode();
    node->setTexture(painted.window->createTextureFromImage(image));
    node->setOwnsTexture(true);
    node->setFiltering(QSGTexture::Linear);
    node->setRect(rect);
    return node;
}

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
// transparents les objets sous elle, redessin immédiat). Limite de DA11 (écart 5, H10-8) : sinon, redessin
// immédiat (reconstruction). La grille du dessus n'est plus une cause (L4-T7) : surcouche hors de cette scène
// (GridOverlayItem), au-dessus de la couche d'attente comme du cache.
bool canWait(const core::PageObject& object)
{
    const auto* stroke = std::get_if<core::Stroke>(&object);
    const bool revealer = stroke && stroke->kind == core::StrokeKind::Revealer;
    return !usesImages(object) && !revealer;
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
    // L5-C4 : un objet inséré en fin de page dans ce même tour (suite d'une commande : collage, ordre) est lu
    // par le redessin que cette reconstruction demande ; le mettre en attente ferait un second redessin, au
    // versement.
    if (samePage &&
        !m_restartedThisTurn) { // pas setPage ni setSource : ils ouvrent une page, qui reste lente
        m_restartedThisTurn = true;
        QMetaObject::invokeMethod(this, [this] { m_restartedThisTurn = false; }, Qt::QueuedConnection);
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
        m_heldDecodeDone = false; // un décodage retenu et fini est remplacé par celui-ci
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
    if (atEnd && m_deferred && m_ready && !m_restartedThisTurn && canWait(object)) {
        // Lot 10 : il attend au-dessus du cache ; ni nœud dans la scène, ni redessin du cache. Limite de DA11
        // (écart 5, H10-8) : hors fin de page, ou objet qui ne peut pas attendre → redessin immédiat
        // ci-dessous.
        m_pendingIds.push_back(id);
        announceAppended();
        emit pendingChanged();
        return;
    }
    // Une encre révélatrice change ce qui est sous elle (F-255) : reconstruction, jamais un ajout en fin de
    // scène.
    if (atEnd && index >= m_builtCount && !imageChanged && m_pendingIds.empty() && !isRevealer(object)) {
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
    if (pending && pendingAreTheLastObjects() && canWait(shown.objects[index])) {
        ++m_pendingRevision; // modifié en place pendant qu'il attend : la couche le refait
        emit pendingChanged();
        return;
    }
    // Hors de l'attente, objet qui ne peut plus attendre (canWait : image, encre révélatrice), ou objet en
    // attente réordonné (annoncé en ObjectChanged, undo.h n'a pas de type
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
    emit rasterScaleChanged();
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

bool FixedPageItem::showHiddenTranslucent() const noexcept
{
    return m_showHiddenTranslucent;
}

void FixedPageItem::setShowHiddenTranslucent(bool on)
{
    if (m_showHiddenTranslucent == on) {
        return;
    }
    m_showHiddenTranslucent = on;
    m_rebuild = true; // une reconstruction par bascule, et un redessin du cache de la vue (contentChanged)
    update();
    emit showHiddenTranslucentChanged();
    emit contentChanged();
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
        QSGNode* fresh = nullptr;
        if (slot.object == kBackgroundSlot) {
            fresh = createBackgroundNode(page, painted);
        } else if (slot.object == kGridSlot) {
            fresh = createGridNode(page, painted);
        } else if (slot.object == kRevealSlot) {
            // Comme le fond et la grille : l'échelle posée par la vue (L8-C6), jamais celle de l'élément.
            fresh = createRevealedNode(page, revealRange(page, cachedObjectCount()), m_images, painted,
                                       m_showHiddenTranslucent, painted.rasterScale);
        } else {
            fresh = m_painted.createNode(page.objects[slot.object], painted);
        }
        if (!fresh) {
            continue;
        }
        QSGNode* parent = slot.node->parent();
        parent->insertChildNodeBefore(fresh, slot.node);
        parent->removeChildNode(slot.node);
        delete slot.node;
        slot.node = fresh;
        ++m_rasterizationCount;
    }
}

void FixedPageItem::setDecodeHeldForTesting(bool held)
{
    m_decodeHeld = held;
    if (!held && m_heldDecodeDone) {
        m_heldDecodeDone = false;
        onImagesReady();
    }
}

void FixedPageItem::onImagesReady()
{
    if (m_decodeHeld) {
        m_heldDecodeDone = true;
        return;
    }
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
    const NodeContexts contexts = nodeContexts(window(), software, &m_images, m_rasterScale,
                                               m_forceRasterized, m_showHiddenTranslucent);
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
        m_rasterSlots.clear();
        if (m_ready) {
            QSGNode* background = createBackgroundNode(shown, painted);
            root->appendChildNode(background);
            if (rasterized && backgroundIsPainted(shown)) {
                addRasterSlot(background, kBackgroundSlot);
            }
            // Grille du dessous seulement : celle du dessus est la surcouche GridOverlayItem, hors de la
            // scène que le cache photographie (L4-T7) ; la masquer ou la montrer ne reconstruit pas cette
            // scène.
            QSGNode* grid =
                core::gridDrawsUnderObjects(shown.grid) ? createGridNode(shown, painted) : nullptr;
            if (grid) {
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
        // Seul setRasterScale (le zoom posé par la vue) refait les nœuds rastérisés, part découpée comprise :
        // aucun redessin d'un zoom continu ne rastérise la page (ui/src/pageview.cpp:1349-1353).
        rescaleRasterNodes(shown, painted);
    }
    m_rescale = false;
    if (m_ready) {
        // Ordre du vecteur (couche, profondeur, arbre) ; moteur du prototype s'il sait faire, sinon moteur
        // peint. Lot 10 : la scène s'arrête aux objets en attente (dessinés par la couche d'attente) ; elle
        // les ajoute quand ils sont versés.
        const std::size_t limit = cachedObjectCount();
        const RevealRange reveal = revealRange(shown, limit);
        for (std::size_t i = m_builtCount; i < limit; ++i) {
            if (reveal.any && i >= reveal.first && i <= reveal.last) {
                if (i == reveal.first) { // F-255 : la part découpée, en un nœud du peintre partagé
                    if (QSGNode* node = createRevealedNode(shown, reveal, m_images, painted,
                                                           m_showHiddenTranslucent, painted.rasterScale)) {
                        if (rasterized) {
                            addRasterSlot(node, kRevealSlot);
                        }
                        root->appendChildNode(node);
                    }
                }
                continue;
            }
            if (isRevealer(shown.objects[i])) {
                continue; // jamais dessinée seule (masquée, ou hors de la couche du dessus : rien à voir)
            }
            const ObjectNode made = createObjectNode(shown.objects[i], i, m_registry, m_painted, contexts);
            if (!made.root) {
                continue;
            }
            if (made.viaPainter && rasterized) {
                // feuille : les nœuds de transformation et d'opacité restent en place
                addRasterSlot(made.leaf, i);
            }
            root->appendChildNode(made.root);
        }
        m_builtCount = limit;
    }
    m_rasterNodes.store(static_cast<int>(m_rasterSlots.size()));
    return root;
}

} // namespace actcraft::render
