#include "penadapters.h"

#include <actcraft/core/clock.h>
#include <actcraft/input/pipeline.h>
#include <actcraft/input/qtevents.h>
#include <actcraft/tools/pentool.h>
#include <actcraft/tools/tool.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QMouseEvent>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QTabletEvent>
#include <QTouchEvent>
#include <QWheelEvent>

#include <algorithm>

namespace actcraft::ui {

namespace {

input::Point2 toPoint2(QPointF p)
{
    return {p.x(), p.y()};
}

// Même défilement, aux arrondis près : l'axe limitant d'« Ajuster » tombe pile sur la marge, et clampPan peut
// l'en écarter d'un rien.
bool samePan(QPointF a, QPointF b)
{
    constexpr double kSamePan = 1e-6;
    return qAbs(a.x() - b.x()) < kSamePan && qAbs(a.y() - b.y()) < kSamePan;
}

// Gestionnaire de pointeur actif qui prend l'appui (noms des classes publiques des types QML). Le survol
// (HoverHandler), la molette (WheelHandler) et le pincement (deux points, jamais un stylet) ne le prennent
// pas.
bool isPressHandler(const QObject* object)
{
    return (object->inherits("QQuickTapHandler") || object->inherits("QQuickDragHandler") ||
            object->inherits("QQuickPointHandler")) &&
           object->property("enabled").toBool();
}

// Types qui prennent eux-mêmes le bouton gauche qu'ils acceptent, qu'ils portent des gestionnaires ou non.
bool takesLeftButtonItself(const QQuickItem* item)
{
    return item->inherits("QQuickMouseArea") || item->inherits("QQuickControl") ||
           item->inherits("QQuickFlickable");
}

// `item` prendrait lui-même un appui : il accepte le toucher, porte un gestionnaire qui prend l'appui, ou
// prend le bouton gauche. API publique seulement : acceptedMouseButtons() compte tous les boutons dès qu'un
// gestionnaire est posé, même un HoverHandler ; le bouton gauche ne compte alors que pour les types qui le
// prennent eux-mêmes (limite : un autre type à bouton gauche propre et survol n'est pas reconnu).
bool takesPress(const QQuickItem* item)
{
    if (item->acceptTouchEvents()) {
        return true;
    }
    bool hasHandlers = false;
    for (const QObject* child : item->children()) {
        if (child->inherits("QQuickPointerHandler")) {
            if (isPressHandler(child)) {
                return true;
            }
            hasHandlers = true;
        }
    }
    const bool left = item->acceptedMouseButtons() & Qt::LeftButton;
    return left && (!hasHandlers || takesLeftButtonItself(item));
}

// `item` ou l'un de ses descendants prendrait l'appui de la souris ou du doigt au point `scenePos` : visible,
// actif, sous le point, et prenant l'appui (barre de défilement, bouton).
bool takesPressAt(const QQuickItem* item, QPointF scenePos)
{
    if (!item->isVisible() || !item->isEnabled()) {
        return false;
    }
    const bool inside = item->contains(item->mapFromScene(scenePos));
    if (inside && takesPress(item)) {
        return true;
    }
    if (item->clip() && !inside) {
        return false;
    }
    const QList<QQuickItem*> children = item->childItems();
    return std::any_of(children.cbegin(), children.cend(),
                       [scenePos](const QQuickItem* child) { return takesPressAt(child, scenePos); });
}

// Lot 10 : une entrée qui agit (appui, relâché, mouvement en contact, molette, appui de touche) repousse le
// versement ; un survol non (stylet tenu près du tableau, souris sur une barre), sinon il ne partirait qu'au
// plafond.
bool isActingInput(const QEvent& event)
{
    switch (event.type()) {
    case QEvent::TabletPress:
    case QEvent::TabletRelease:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::Wheel:
    case QEvent::KeyPress:
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        return true;
    case QEvent::TabletMove: {
        const auto& tablet = static_cast<const QTabletEvent&>(event);
        return tablet.buttons() != Qt::NoButton || tablet.pressure() > 0;
    }
    case QEvent::MouseMove:
        return static_cast<const QMouseEvent&>(event).buttons() != Qt::NoButton;
    default:
        return false;
    }
}

} // namespace

// Fin du Pipeline (D-S1, D-S9) : l'outil qui reçoit la voie Tool maintenant (outil de ToolHost, sinon stylo
// interne). Lu à chaque échantillon : un changement d'outil ne reconstruit rien.
class ToolDispatch final : public input::SampleSink
{
public:
    explicit ToolDispatch(const PageView& view) : m_view(view) {}
    void onSample(const input::Sample& sample) override
    {
        if (tools::Tool* tool = m_view.currentTool()) {
            tool->onSample(sample);
        }
    }

private:
    const PageView& m_view;
};

PageView::PageView(QQuickItem* parent) : QQuickItem(parent), m_dispatch(std::make_unique<ToolDispatch>(*this))
{
    // Bouton droit : menu contextuel par signal (contextMenuRequested), jamais à l'outil.
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setAcceptTouchEvents(true);
    m_cacheTimer.setSingleShot(true);
    m_cacheTimer.setInterval(kCacheRefreshDelayMs);
    connect(&m_cacheTimer, &QTimer::timeout, this, [this] {
        updateCacheTextureSize();
        requestCacheRefresh();
    });
    m_foldTimer.setSingleShot(true);
    m_foldTimer.setTimerType(Qt::PreciseTimer); // la reprise de « pauses » vise la fin du délai à 8 ms près
    m_foldTimer.setInterval(kCacheFoldIdleMs);
    connect(&m_foldTimer, &QTimer::timeout, this, [this] {
        // D-S9 : jamais pendant un geste de l'outil qui reçoit la voie Tool (trait, sélection, gomme).
        if (const auto* busy = currentTool(); busy && busy->isBusy()) {
            m_foldTimer.start();
            return;
        }
        if (m_fixedPage && m_fixedPage->pendingCount() > 0) {
            requestCacheRefresh(); // verse et redessine, hors de l'écriture
        }
    });
    // Une couverture est une route de la politique (D-S1) : le routeur la consulte à l'appui, pour le
    // stylet, le doigt et la souris ; aucune autre route ne la teste.
    m_router.setCoverTest([this](const input::Sample& press) {
        return coveredAt(m_transform.pageToView(QPointF(press.x, press.y)));
    });
}

PageView::~PageView() = default;

QQuickItem* PageView::content() const noexcept
{
    return m_content;
}

void PageView::setContent(QQuickItem* content)
{
    if (content == m_content) {
        return;
    }
    const QSizeF oldPage = pageSize();
    const ViewTransform old = m_transform;
    disconnect(m_contentWidthConnection);
    disconnect(m_contentHeightConnection);
    m_content = content;
    m_lastPageSize = pageSize();
    if (m_content) {
        m_content->setTransformOrigin(QQuickItem::TopLeft);
        // La taille du contenu change sans changer d'élément (autre page, autre document) : barres, zoom et
        // cache suivent.
        m_contentWidthConnection = connect(m_content, &QQuickItem::widthChanged, this,
                                           [this] { onContentSizeChanged(m_lastPageSize); });
        m_contentHeightConnection = connect(m_content, &QQuickItem::heightChanged, this,
                                            [this] { onContentSizeChanged(m_lastPageSize); });
    }
    if (m_fitMode) {
        fitPage();
    } else {
        apply(clampPan(m_transform, size(), pageSize()));
    }
    // La taille de la page change les barres même quand ni le zoom ni le défilement ne bougent (apply n'a
    // alors rien émis).
    if (pageSize() != oldPage && qFuzzyCompare(old.zoom, m_transform.zoom) && old.pan == m_transform.pan) {
        emit scrollChanged();
    }
    emit contentChanged();
}

qreal PageView::zoom() const noexcept
{
    return m_transform.zoom;
}

void PageView::setZoom(qreal zoom)
{
    m_fitMode = false;
    m_cacheTimer.stop();
    const ViewTransform t =
        clampPan(zoomAt(m_transform, QPointF(width() / 2, height() / 2), zoom), size(), pageSize());
    const bool changed = !qFuzzyCompare(t.zoom, m_transform.zoom);
    apply(t);
    if (changed) {
        updateCacheTextureSize();
        requestCacheRefresh();
    }
}

QPointF PageView::pan() const noexcept
{
    return m_transform.pan;
}

void PageView::setPan(QPointF pan)
{
    m_fitMode = false;
    ViewTransform t = m_transform;
    t.pan = pan;
    apply(clampPan(t, size(), pageSize()));
}

bool PageView::cacheEnabled() const noexcept
{
    return m_cacheEnabled;
}

void PageView::setCacheEnabled(bool enabled)
{
    if (enabled == m_cacheEnabled) {
        return;
    }
    m_cacheEnabled = enabled;
    updateDeferral();
    emit cacheEnabledChanged();
    requestCacheRefresh();
}

QSize PageView::cacheTextureSize() const noexcept
{
    return m_cacheTextureSize;
}

int PageView::cacheSamples() const noexcept
{
    return m_cacheSamples;
}

void PageView::setCacheSamples(int samples)
{
    samples = std::max(1, samples);
    if (samples == m_cacheSamples) {
        return;
    }
    m_cacheSamples = samples;
    emit cacheSamplesChanged();
    updateCacheTextureSize();
    requestCacheRefresh();
}

bool PageView::cacheReady() const noexcept
{
    return m_cacheReady;
}

const ViewTransform& PageView::viewTransform() const noexcept
{
    return m_transform;
}

void PageView::fitPage()
{
    m_fitMode = true;
    m_cacheTimer.stop();
    const ViewTransform t = ui::fitPage(size(), pageSize());
    const bool changed = !qFuzzyCompare(t.zoom, m_transform.zoom) || m_cacheTextureSize.isEmpty();
    apply(t);
    if (changed) {
        updateCacheTextureSize();
        requestCacheRefresh();
    }
}

void PageView::requestCacheRefresh()
{
    // Lot 10 : tout redessin verse, dans le même tour que la demande (jamais à sa fin : un objet inséré entre
    // la demande et la synchronisation suivante ne serait alors ni dans le cache ni dans la couche
    // d'attente).
    m_foldTimer.stop();
    if (m_fixedPage) {
        m_fixedPage->foldPending();
    }
    if (!m_cacheEnabled) {
        m_refreshInFlight = false;
        m_refreshAgain = false;
        m_refreshThisTurn = false;
        setCacheReady(true);
        emit cacheUpdated();
        return;
    }
    setCacheReady(false);
    if (m_refreshThisTurn) {
        // M3 : une demande de ce tour est déjà partie ; son redessin ne commence qu'à la synchronisation
        // suivante, après ce tour : il couvre celle-ci (soulèvement ou dépôt de plusieurs objets : un seul).
        return;
    }
    if (m_refreshInFlight) {
        m_refreshAgain = true; // arrivée après le départ du redessin en cours : sa fin ne suffira pas
    }
    m_refreshInFlight = true;
    holdRequestsOfThisTurn();
    emit cacheRefreshRequested(); // la première du tour part tout de suite (décision 10)
}

void PageView::holdRequestsOfThisTurn()
{
    if (m_refreshThisTurn) {
        return;
    }
    m_refreshThisTurn = true;
    // Le tour suivant commence : le redessin parti a pu commencer, une demande de plus en part un autre. La
    // synchronisation d'un UpdateRequest posté plus tôt dans ce tour passe avant ce lambda : elle remet aussi
    // le drapeau à faux (itemChange, m4).
    QMetaObject::invokeMethod(this, [this] { m_refreshThisTurn = false; }, Qt::QueuedConnection);
}

void PageView::notifyCacheUpdated()
{
    // Ce redessin est fini : une demande de plus, même dans ce tour, en part un autre.
    m_refreshThisTurn = false;
    if (m_refreshAgain) {
        // Ce redessin a pu commencer avant la dernière demande : un de plus garantit l'état final.
        m_refreshAgain = false;
        holdRequestsOfThisTurn();
        emit cacheRefreshRequested();
        return;
    }
    m_refreshInFlight = false;
    ++m_cacheRefreshCount; // redessin terminé (une relance, plus haut, ne compte pas)
    setCacheReady(true);
    emit cacheUpdated();
}

render::FixedPageItem* PageView::fixedPage() const noexcept
{
    return m_fixedPage;
}

void PageView::setFixedPage(render::FixedPageItem* page)
{
    if (page == m_fixedPage) {
        return;
    }
    // Lot 10 : l'ancienne page ne reste pas en différé, où personne ne verserait plus ce qui y attend.
    applyDeferral(false);
    disconnect(m_readyConnection);
    disconnect(m_contentConnection);
    disconnect(m_pendingConnection);
    m_fixedPage = page;
    if (m_fixedPage) {
        // Page prête (premier dessin, images décodées) ou contenu changé (trait inséré, annulation, autre
        // page lue) : un redessin du cache. Pas avant que la page soit prête : readyChanged suivra.
        const auto refresh = [this] {
            if (m_fixedPage && m_fixedPage->isReady()) {
                requestCacheRefresh();
            }
        };
        m_readyConnection = connect(m_fixedPage, &render::FixedPageItem::readyChanged, this, refresh);
        m_contentConnection = connect(m_fixedPage, &render::FixedPageItem::contentChanged, this, refresh);
        // Lot 10 : un objet mis en attente (ou retiré pendant qu'il attend) n'émet que pendingChanged.
        m_pendingConnection =
            connect(m_fixedPage, &render::FixedPageItem::pendingChanged, this, &PageView::onPendingChanged);
    }
    updateDeferral();
    rebuildPen();
    emit fixedPageChanged();
}

render::PendingLayerItem* PageView::pendingLayer() const noexcept
{
    return m_pendingLayer;
}

void PageView::setPendingLayer(render::PendingLayerItem* layer)
{
    if (layer == m_pendingLayer) {
        return;
    }
    disconnect(m_layerPageConnection);
    disconnect(m_layerDestroyedConnection);
    m_pendingLayer = layer;
    if (layer) {
        // Sa page fixe peut lui être donnée après la vue (ordre des liaisons QML) ou changer.
        m_layerPageConnection =
            connect(layer, &render::PendingLayerItem::fixedPageChanged, this, &PageView::updateDeferral);
        // Détruite : plus personne ne dessine ce qui attend, la page fixe quitte le différé.
        m_layerDestroyedConnection = connect(layer, &QObject::destroyed, this, [this] {
            m_pendingLayer = nullptr;
            updateDeferral();
            emit pendingLayerChanged();
        });
    }
    updateDeferral();
    emit pendingLayerChanged();
}

bool PageView::foldTimerActive() const noexcept
{
    return m_foldTimer.isActive();
}

void PageView::updateDeferral()
{
    // La couche doit dessiner les objets en attente de CETTE page fixe : regardant une autre page (ou
    // aucune), personne ne les dessinerait.
    const bool deferred = m_cacheEnabled && !m_pendingLayer.isNull() && m_fixedPage &&
                          m_pendingLayer->fixedPage() == m_fixedPage;
    applyDeferral(deferred);
    if (!deferred) {
        m_foldTimer.stop();
    }
}

void PageView::applyDeferral(bool deferred)
{
    if (!m_fixedPage) {
        return;
    }
    if (!deferred && m_fixedPage->pendingCount() > 0) {
        requestCacheRefresh(); // ce qui attendait passe au cache avant que la couche ne serve plus
    }
    m_fixedPage->setDeferredFolding(deferred);
}

void PageView::onPendingChanged()
{
    if (!m_fixedPage || m_fixedPage->pendingCount() == 0) {
        m_foldTimer.stop();
        return;
    }
    if (m_fixedPage->pendingCount() >= kCacheFoldMaxPending) {
        requestCacheRefresh(); // plafond : versé tout de suite (H10-3)
        return;
    }
    m_foldTimer.start();
}

void PageView::postponeFold()
{
    if (m_foldTimer.isActive()) {
        m_foldTimer.start();
    }
}

render::InkItem* PageView::ink() const noexcept
{
    return m_ink;
}

void PageView::setInk(render::InkItem* ink)
{
    if (ink == m_ink) {
        return;
    }
    m_ink = ink;
    rebuildPen();
    emit inkChanged();
}

void PageView::setInkProbe(core::InkProbe* probe)
{
    m_probe = probe;
    if (m_pipeline) {
        m_pipeline->setProbe(probe);
    }
    if (m_ink) {
        m_ink->setProbe(probe);
    }
}

void PageView::resetInputSequence(std::uint64_t next)
{
    if (m_pipeline) {
        m_pipeline->resetSequence(next);
    }
}

DocumentController* PageView::documentController() const noexcept
{
    return m_controller.data();
}

void PageView::setDocumentController(DocumentController* controller)
{
    if (controller == m_controller) {
        return;
    }
    disconnect(m_controllerConnection);
    m_controller = controller;
    if (m_controller) {
        // Contrôleur détruit avant la vue : le stylo ne garde pas de référence pendante.
        m_controllerConnection = connect(m_controller, &QObject::destroyed, this, [this] { rebuildPen(); });
    }
    rebuildPen();
    emit documentControllerChanged();
}

void PageView::onStrokeFrozen()
{
    emit strokeFrozen(); // le cache est redessiné sur contentChanged de la page fixe (un redessin par trait)
}

void PageView::rebuildPen()
{
    m_pipeline.reset();
    m_pen.reset();
    m_documentSink.reset();
    m_controllerSink.reset();
    m_inkSurface.reset();
    if (!m_fixedPage || !m_ink) {
        return;
    }
    m_inkSurface = std::make_unique<InkSurfaceAdapter>(*m_ink);
    actcraft::tools::DocumentSink* sink = nullptr;
    if (m_controller) {
        m_controllerSink = std::make_unique<ControllerSink>(*m_controller, *this);
        sink = m_controllerSink.get();
    } else {
        m_documentSink = std::make_unique<FixedPageSink>(*m_fixedPage, *this);
        sink = m_documentSink.get();
    }
    m_pen = std::make_unique<actcraft::tools::PenTool>(*m_inkSurface, *sink);
    m_pen->setColor(m_inkColor.rgba());
    m_pen->setBaseWidth(m_inkWidth);
    m_pipeline = std::make_unique<input::Pipeline>(*m_dispatch); // vers currentTool()
    m_pipeline->setProbe(m_probe);
    m_ink->setProbe(m_probe);
}

bool PageView::handlePointer(const QEvent& event, QPointF viewPos)
{
    if (isActingInput(event)) {
        postponeFold();
    }
    if (viewPos != m_pointerPosition) { // rien à signaler quand rien ne bouge
        m_pointerPosition = viewPos;
        emit pointerPositionChanged();
    }
    if (!m_pipeline) {
        return false;
    }
    const auto& pointer = static_cast<const QPointerEvent&>(event);
    if (!acceptsDevice(pointer.pointingDevice())) {
        return false;
    }
    const QPointF pagePos = viewTransform().viewToPage(viewPos);
    auto sample = input::sampleFromEvent(event, pagePos, core::monotonicNowNs());
    if (!sample) {
        return false;
    }
    const input::Decision decision = m_router.route(*sample);
    // Amendement 1 du lot 4 (V5) : contactActive suit les décisions que le routeur retient.
    const bool released = sample->phase == input::Phase::Release;
    if (sample->phase == input::Phase::Press &&
        (decision.route == input::Route::Tool || decision.route == input::Route::Gesture)) {
        holdContact(sample->pointer);
    }
    if (decision.endToolContacts) {
        endToolContacts(decision); // décidé par la politique du routeur : la vue ne compte pas les doigts
    }
    switch (decision.route) {
    case input::Route::Tool:
        break;
    case input::Route::Gesture: {
        const bool taken = onGestureSample(*sample, viewPos);
        if (released) {
            forgetContact(sample->pointer); // décision rendue par le routeur au relâché
        }
        return taken;
    }
    case input::Route::Ignore:
    case input::Route::Cover:
        // La vue ne prend rien et rend aussitôt le contact au routeur ; l'événement continue vers la
        // surcouche, qui le reçoit par ses gestionnaires.
        releaseContact(sample->pointer);
        return false;
    }
    sample->user = decision.user;
    m_lastRawPagePoint = pagePos;
    if (sample->device == input::DeviceKind::Touch) {
        m_lastTouchPagePoint = pagePos;
        m_lastTouchViewPoint = viewPos;
        m_touchPointer = sample->pointer;
        m_touchDrawing = sample->phase != input::Phase::Release;
    }
    pushToTool(*sample);
    if (released) {
        forgetContact(sample->pointer); // après le relâché versé à l'outil (trait déjà au document)
    }
    return true;
}

void PageView::endToolContacts(const input::Decision& decision)
{
    if (!m_touchDrawing) {
        return;
    }
    if (decision.route == input::Route::Gesture) {
        // H3-17 : le deuxième doigt pose un geste, le doigt qui écrivait y passe (routeur) ; son trait est
        // abandonné, rien n'est versé ni annulable.
        abandonTouchStroke();
        addGestureContact(m_touchPointer, m_lastTouchViewPoint);
    } else {
        // M4 (b) : un stylet ou la souris prend la main. Le trait du doigt se termine là où il était ; le
        // routeur garde le doigt (Ignore) : la suite de son contact n'écrit plus, la vue n'a rien à oublier.
        terminateTouchStroke();
    }
}

void PageView::pushToTool(const input::Sample& sample)
{
    if (m_filter) {
        m_pipeline->push(m_filter(sample));
    } else {
        m_pipeline->push(sample);
    }
}

void PageView::finishStroke()
{
    // Report de L3-T14 : un geste d'un outil de ToolHost (glissement de la sélection, gomme) est d'abord
    // abandonné sans étape, comme au changement de page (Main.qml) ; le relâché synthétisé plus bas le
    // terminerait par un déplacement. Le stylo de ToolHost y verse son trait (Tool::cancel).
    if (m_tools) {
        m_tools->cancelGesture();
    }
    if (m_touchDrawing) {
        m_touchDrawing = false;         // la suite du contact du doigt ne dessine plus
        releaseContact(m_touchPointer); // ni ne reste retenue au routeur : la vue n'en verra pas le relâché
    }
    // Stylet : m_stylusDrawing reste vrai, le contact reste pris jusqu'au relâché (rien n'en passe à
    // l'interface) ; ses points n'ajoutent rien, l'outil n'est plus occupé. Souris : idem par la saisie.
    auto* tool = currentTool();
    if (!m_pipeline || !tool || !tool->isBusy()) {
        return;
    }
    input::Sample release;
    release.x = m_lastRawPagePoint.x(); // au dernier point brut, filtré comme lui : aucun point de plus
    release.y = m_lastRawPagePoint.y();
    release.timestampNs = core::monotonicNowNs();
    release.phase = input::Phase::Release;
    pushToTool(release);
    if (tool->isBusy()) {
        tool->cancel(); // un outil que le relâché ne termine pas (L3-T14) finit son geste proprement
    }
}

void PageView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        // Test de couverture direct : sampleFromEvent ne rend rien pour le bouton droit, le routeur n'est
        // pas consulté. Rien au Pipeline ; la suite du contact droit n'a pas d'échantillon (ignorée).
        if (coveredAt(event->position())) {
            event->ignore();
            return;
        }
        event->accept();
        emit contextMenuRequested(event->position());
        return;
    }
    if (!canStartStroke(event->position())) {
        event->ignore();
        return;
    }
    event->setAccepted(handlePointer(*event, event->position()));
}

void PageView::mouseMoveEvent(QMouseEvent* event)
{
    event->setAccepted(handlePointer(*event, event->position()));
}

void PageView::mouseReleaseEvent(QMouseEvent* event)
{
    event->setAccepted(handlePointer(*event, event->position()));
}

// Variante B (T2 : tablette-element = non) : le stylet n'est pas adressé aux éléments, la fenêtre le reçoit.
bool PageView::event(QEvent* event)
{
    return QQuickItem::event(event);
}

void PageView::itemChange(ItemChange change, const ItemChangeData& value)
{
    QQuickItem::itemChange(change, value);
    if (change == ItemSceneChange) {
        if (m_filteredWindow) {
            m_filteredWindow->removeEventFilter(this);
            disconnect(m_filteredWindow, &QQuickWindow::afterSynchronizing, this, nullptr);
        }
        m_filteredWindow = value.window;
        if (m_filteredWindow) {
            m_filteredWindow->installEventFilter(this);
            // m4 (L3-C8) : le redessin parti a lu la page à cette synchronisation (le cache, live: false, est
            // rendu dans l'image qui la suit) ; une demande de plus, même dans ce tour, en part un autre.
            // Fil de rendu en boucle à fils : le fil principal est alors bloqué, rien d'autre ne lit l'état.
            connect(
                m_filteredWindow, &QQuickWindow::afterSynchronizing, this,
                [this] { m_refreshThisTurn = false; }, Qt::DirectConnection);
            // Moteur de la fenêtre (H10-9) : QQuickWindow::graphicsApi() ne renvoie jamais Software en
            // Qt 6.12 (il suit QSGRhiSupport) ; rendererInterface()->graphicsApi() répond dès la construction
            // de la fenêtre (même usage : tests/support/renderhost.h). samples est sans effet en rendu
            // logiciel.
            const int samples =
                m_filteredWindow->rendererInterface()->graphicsApi() == QSGRendererInterface::Software
                    ? 1
                    : kCacheMsaaSamples;
            if (samples != m_cacheSamples) {
                m_cacheSamples = samples;
                emit cacheSamplesChanged();
            }
        }
        // Rapport de pixels connu à l'attache : sans ceci, une taille calculée avant garde l'ancienne
        // jusqu'au prochain zoom (H10-9).
        updateCacheTextureSize();
    }
}

bool PageView::eventFilter(QObject* watched, QEvent* event)
{
    if (isActingInput(*event)) {
        postponeFold(); // lot 10 : rien n'est pris ni filtré de plus pour autant
    }
    const QEvent::Type type = event->type();
    if (type == QEvent::TabletPress || type == QEvent::TabletMove || type == QEvent::TabletRelease) {
        auto* tablet = static_cast<QTabletEvent*>(event);
        const QPointF viewPos = mapFromScene(tablet->position());
        // La décision se prend à l'appui, comme la saisie implicite de Qt Quick pour la souris : un trait ne
        // commence que sur la vue ; commencé, il suit le stylet partout jusqu'au relâché. Le reste (appui
        // ailleurs et ce qui le suit, survol) passe au reste de l'interface. Le trait suivi est celui du
        // stylet, pas celui du stylo (PenTool::isDrawing vaut aussi pour un doigt ou la souris).
        bool taken = false;
        if (type == QEvent::TabletPress) {
            if (canStartStroke(viewPos) && acceptsDevice(tablet->pointingDevice())) {
                // Un seul trait à la fois : si un doigt écrit, la politique du routeur le fait passer à
                // Ignore (endToolContacts) et handlePointer termine son trait ; un appui couvert (Cover) ne
                // le termine pas.
                taken = handlePointer(*tablet, viewPos);
            }
            m_stylusDrawing = taken;
        } else if (m_stylusDrawing) {
            taken = handlePointer(*tablet, viewPos);
            if (type == QEvent::TabletRelease) {
                m_stylusDrawing = false;
            }
        }
        if (taken) {
            tablet->accept();
            return true; // consommé : pas de souris synthétisée en double
        }
    }
    return QQuickItem::eventFilter(watched, event);
}

bool PageView::canStartStroke(QPointF viewPos) const
{
    // isEnabled : le stylet passe par le filtre de la fenêtre et contournerait « enabled: false ».
    return !m_inputBlocked && isVisible() && isEnabled() && contains(viewPos);
}

bool PageView::coveredAt(QPointF viewPos) const
{
    const QPointF scenePos = mapToScene(viewPos);
    if (const QQuickItem* parent = parentItem()) {
        // Ordre de Qt Quick : z croissant, puis ordre des enfants à z égal.
        const QList<QQuickItem*> siblings = parent->childItems();
        const qsizetype self = siblings.indexOf(this);
        for (qsizetype i = 0; i < siblings.size(); ++i) {
            const QQuickItem* other = siblings.at(i);
            const bool above = other->z() > z() || (!(other->z() < z()) && i > self);
            if (above && takesPressAt(other, scenePos)) {
                return true;
            }
        }
    }
    // Couvertures déclarées (D-S1) : visibles, actives, sous le point (containmentMask respecté).
    return std::any_of(m_covers.cbegin(), m_covers.cend(), [scenePos](const QPointer<QQuickItem>& cover) {
        return cover && cover->isVisible() && cover->isEnabled() &&
               cover->contains(cover->mapFromScene(scenePos));
    });
}

void PageView::touchEvent(QTouchEvent* event)
{
    if (event->type() == QEvent::TouchCancel) {
        finishTouchStroke(); // annulation du système : le trait du doigt est versé à son dernier point
        endGesture();
        event->accept();
        return;
    }
    // L'événement remis à la vue ne porte que ses points (saisis par elle ou appuyés sur elle). Chaque point
    // est un contact du routeur, converti comme un événement d'un seul point : la politique dit où il va
    // (outil, geste, rien : paume, troisième doigt) ; la vue ne compte pas les doigts.
    bool taken = false;
    for (const QEventPoint& point : event->points()) {
        QEvent::Type type = QEvent::None;
        if (point.state() == QEventPoint::State::Pressed) {
            type = QEvent::TouchBegin;
        } else if (point.state() == QEventPoint::State::Updated) {
            type = QEvent::TouchUpdate;
        } else if (point.state() == QEventPoint::State::Released) {
            type = QEvent::TouchEnd;
        } else {
            continue; // immobile : rien de neuf pour le routeur
        }
        if (type == QEvent::TouchBegin && !canStartStroke(point.position())) {
            continue;
        }
        const QTouchEvent single(type, event->pointingDevice(), event->modifiers(), {point});
        taken = handlePointer(single, point.position()) || taken;
    }
    // Un seul pas du geste par événement, une fois tous ses points connus : deux doigts déplacés ensemble ne
    // font pas un zoom passager (le premier seul aurait changé l'écart).
    if (m_gestureMoved && m_gesture.active()) {
        applyGesture();
    }
    m_gestureMoved = false;
    event->setAccepted(taken);
}

void PageView::touchUngrabEvent()
{
    finishTouchStroke(); // saisie volée (Flickable) : le trait du doigt se termine proprement
    endGesture();
    QQuickItem::touchUngrabEvent();
}

bool PageView::onGestureSample(const input::Sample& sample, QPointF viewPos)
{
    switch (sample.phase) {
    case input::Phase::Press:
        if (!addGestureContact(sample.pointer, viewPos)) {
            releaseContact(sample.pointer); // une autre politique en enverrait un troisième : rien
            return false;
        }
        break;
    case input::Phase::Move:
        for (GestureContact& contact : m_gestureContacts) {
            if (contact.pointer == sample.pointer) {
                contact.viewPos = viewPos;
                m_gestureMoved = true;
            }
        }
        break;
    case input::Phase::Release:
        // Un des deux se relève : le geste finit ; l'autre reste au geste (routeur) et en recommence un avec
        // un nouveau doigt.
        for (GestureContact& contact : m_gestureContacts) {
            if (contact.pointer == sample.pointer) {
                contact = GestureContact{};
            }
        }
        stopGesture();
        break;
    }
    return true;
}

bool PageView::addGestureContact(std::uint64_t pointer, QPointF viewPos)
{
    GestureContact* slot = nullptr;
    for (GestureContact& contact : m_gestureContacts) {
        if (contact.pointer == pointer) {
            slot = &contact;
        }
    }
    for (GestureContact& contact : m_gestureContacts) {
        if (!slot && contact.pointer == 0) {
            slot = &contact;
        }
    }
    if (!slot) {
        return false;
    }
    slot->pointer = pointer;
    slot->viewPos = viewPos;
    const bool complete = std::all_of(m_gestureContacts.cbegin(), m_gestureContacts.cend(),
                                      [](const GestureContact& contact) { return contact.pointer != 0; });
    if (complete && !m_gesture.active()) {
        const input::Point2 a = toPoint2(m_gestureContacts[0].viewPos);
        const input::Point2 b = toPoint2(m_gestureContacts[1].viewPos);
        m_gesture.begin(a, b);
        const input::GestureStep step = m_gesture.update(a, b);
        m_gestureZoom = m_transform.zoom;
        m_gesturePage = m_transform.viewToPage(QPointF(step.anchor.x, step.anchor.y));
        emit gestureActiveChanged();
    }
    return true;
}

void PageView::applyGesture()
{
    const input::Point2 a = toPoint2(m_gestureContacts[0].viewPos);
    const input::Point2 b = toPoint2(m_gestureContacts[1].viewPos);
    const input::GestureStep step = m_gesture.update(a, b);
    // Pincer : zoom au début du geste × écart relatif, par le zoom ancré unique (borné comme la molette,
    // quitte « Ajuster », cache redessiné après le dernier pas), autour du point de page de l'ancre.
    zoomAround(m_transform.pageToView(m_gesturePage), m_gestureZoom * step.scale);
    // Défiler : ce point de page est ramené sous le milieu des doigts.
    ViewTransform t = m_transform;
    t.pan += QPointF(step.center.x, step.center.y) - m_transform.pageToView(m_gesturePage);
    t = clampPan(t, size(), pageSize());
    if (!samePan(t.pan, m_transform.pan)) {
        m_fitMode = false;
        apply(t);
    }
}

void PageView::endGesture()
{
    for (GestureContact& contact : m_gestureContacts) {
        if (contact.pointer != 0) {
            releaseContact(contact.pointer);
        }
        contact = GestureContact{};
    }
    m_gestureMoved = false;
    stopGesture();
}

void PageView::stopGesture()
{
    if (m_gesture.active()) {
        m_gesture.end();
        emit gestureActiveChanged();
    }
}

void PageView::abandonTouchStroke()
{
    m_touchDrawing = false;
    if (auto* tool = currentTool(); tool && tool->isBusy()) {
        tool->abandon();
    }
}

void PageView::finishTouchStroke()
{
    if (!m_touchDrawing) {
        return;
    }
    terminateTouchStroke();
    releaseContact(m_touchPointer); // la vue n'en verra pas le relâché
}

void PageView::terminateTouchStroke()
{
    m_touchDrawing = false;
    if (auto* tool = currentTool(); m_pipeline && tool && tool->isBusy()) {
        input::Sample release;
        release.x = m_lastTouchPagePoint.x(); // dernier point brut du doigt, filtré comme lui
        release.y = m_lastTouchPagePoint.y();
        release.timestampNs = core::monotonicNowNs();
        release.device = input::DeviceKind::Touch;
        release.phase = input::Phase::Release;
        release.pointer = m_touchPointer;
        pushToTool(release);
        if (tool->isBusy()) {
            tool->cancel();
        }
    }
}

QColor PageView::inkColor() const noexcept
{
    return m_inkColor;
}

void PageView::setInkColor(const QColor& color)
{
    if (!color.isValid() || color == m_inkColor) {
        return;
    }
    m_inkColor = color;
    if (m_pen) {
        m_pen->setColor(m_inkColor.rgba());
    }
    emit penChanged();
}

qreal PageView::inkWidth() const noexcept
{
    return m_inkWidth;
}

void PageView::setInkWidth(qreal width)
{
    const qreal clamped = std::clamp(width, kMinInkWidth, kMaxInkWidth);
    if (qFuzzyCompare(clamped, m_inkWidth)) {
        return;
    }
    m_inkWidth = clamped;
    if (m_pen) {
        m_pen->setBaseWidth(m_inkWidth);
    }
    emit penChanged();
}

bool PageView::inputBlocked() const noexcept
{
    return m_inputBlocked;
}

void PageView::setInputBlocked(bool blocked)
{
    if (blocked == m_inputBlocked) {
        return;
    }
    m_inputBlocked = blocked;
    emit inputBlockedChanged();
}

qreal PageView::scrollSizeX() const
{
    return scrollAxis(m_transform.pan.x(), width(), pageSize().width() * m_transform.zoom).size;
}

qreal PageView::scrollPositionX() const
{
    return scrollAxis(m_transform.pan.x(), width(), pageSize().width() * m_transform.zoom).position;
}

qreal PageView::scrollSizeY() const
{
    return scrollAxis(m_transform.pan.y(), height(), pageSize().height() * m_transform.zoom).size;
}

qreal PageView::scrollPositionY() const
{
    return scrollAxis(m_transform.pan.y(), height(), pageSize().height() * m_transform.zoom).position;
}

void PageView::scrollTo(qreal positionX, qreal positionY)
{
    ViewTransform t = m_transform;
    const QSizeF content = pageSize() * t.zoom;
    t.pan = QPointF(panForScrollPosition(positionX, width(), content.width()),
                    panForScrollPosition(positionY, height(), content.height()));
    t = clampPan(t, size(), pageSize());
    // Une barre branchée sur la position renvoie souvent la position courante : rien ne bouge, « Ajuster »
    // reste.
    if (samePan(t.pan, m_transform.pan)) {
        return;
    }
    m_fitMode = false;
    apply(t);
}

QPointF PageView::pageToScene(QPointF pagePoint) const
{
    return mapToScene(m_transform.pageToView(pagePoint));
}

void PageView::setExclusiveDevices(QList<const QInputDevice*> devices)
{
    m_exclusive = std::move(devices);
}

bool PageView::acceptsDevice(const QInputDevice* device) const
{
    return m_exclusive.isEmpty() || m_exclusive.contains(device);
}

ToolHost* PageView::tools() const noexcept
{
    return m_tools.data();
}

void PageView::setTools(ToolHost* tools)
{
    if (tools == m_tools) {
        return;
    }
    m_tools = tools; // lu à chaque échantillon (currentTool) : rien à reconstruire
    emit toolsChanged();
}

tools::Tool* PageView::currentTool() const
{
    if (m_tools) {
        if (auto* active = m_tools->activeTool()) {
            return active;
        }
    }
    return m_pen.get();
}

int PageView::cacheRefreshCount() const noexcept
{
    return m_cacheRefreshCount;
}

bool PageView::gestureActive() const noexcept
{
    return m_gesture.active();
}

bool PageView::contactActive() const noexcept
{
    return !m_heldContacts.isEmpty();
}

void PageView::holdContact(std::uint64_t pointer)
{
    const bool was = contactActive();
    if (!m_heldContacts.contains(pointer)) {
        m_heldContacts.append(pointer);
    }
    if (!was) {
        emit contactActiveChanged();
    }
}

void PageView::forgetContact(std::uint64_t pointer)
{
    const bool was = contactActive();
    m_heldContacts.removeAll(pointer);
    if (was && !contactActive()) {
        emit contactActiveChanged();
    }
}

void PageView::releaseContact(std::uint64_t pointer)
{
    m_router.release(pointer);
    forgetContact(pointer);
}

input::PointerRouter& PageView::router() noexcept
{
    return m_router;
}

void PageView::addInputCover(QQuickItem* cover)
{
    if (!cover || m_covers.contains(cover)) {
        return;
    }
    m_covers.append(cover);
    connect(cover, &QObject::destroyed, this, &PageView::forgetDeadCovers, Qt::UniqueConnection);
}

void PageView::removeInputCover(QQuickItem* cover)
{
    if (!cover) {
        return;
    }
    m_covers.removeAll(cover);
    disconnect(cover, &QObject::destroyed, this, &PageView::forgetDeadCovers);
}

void PageView::forgetDeadCovers()
{
    // À destroyed, le QPointer est déjà nul : on retire les couvertures mortes, pas un pointeur donné.
    m_covers.removeIf([](const QPointer<QQuickItem>& cover) { return cover.isNull(); });
}

void PageView::setSampleFilter(SampleFilter filter)
{
    m_filter = std::move(filter);
}

void PageView::zoomAround(QPointF viewPoint, qreal zoom)
{
    // Comme scrollTo : si rien ne bouge (butée du zoom, page entière visible), « Ajuster » reste.
    const ViewTransform t = clampPan(zoomAt(m_transform, viewPoint, zoom), size(), pageSize());
    const bool changed = !qFuzzyCompare(t.zoom, m_transform.zoom);
    if (!changed && samePan(t.pan, m_transform.pan)) {
        return;
    }
    m_fitMode = false;
    apply(t);
    if (changed) {
        setCacheReady(false);
        m_cacheTimer.start(); // redessin kCacheRefreshDelayMs après le dernier appel
    }
}

QPointF PageView::sceneToPage(QPointF scenePoint) const
{
    return m_transform.viewToPage(mapFromScene(scenePoint));
}

void PageView::wheelEvent(QWheelEvent* event)
{
    if (!acceptsDevice(event->pointingDevice())) {
        event->ignore();
        return;
    }
    event->accept();
    // Comme scrollTo : si rien ne bouge (page entière visible, butée du zoom), « Ajuster » reste.
    if (event->modifiers() & Qt::ControlModifier) {
        const int delta = event->angleDelta().y();
        if (delta == 0) {
            return;
        }
        zoomAround(event->position(), m_transform.zoom * wheelZoomFactor(delta)); // zoom ancré unique (M7)
    } else {
        ViewTransform t = m_transform;
        t.pan += wheelScrollDelta(event->angleDelta(), event->pixelDelta(), event->modifiers());
        t = clampPan(t, size(), pageSize());
        if (samePan(t.pan, m_transform.pan)) {
            return;
        }
        m_fitMode = false;
        apply(t);
    }
}

void PageView::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (m_fitMode) {
        fitPage();
    } else {
        apply(clampPan(m_transform, size(), pageSize()));
    }
    emit scrollChanged(); // la taille de la vue change la taille des barres
}

QSizeF PageView::pageSize() const
{
    return m_content ? QSizeF(m_content->width(), m_content->height()) : QSizeF(1024, 768);
}

void PageView::apply(const ViewTransform& transform)
{
    const bool zoomChangedNow = !qFuzzyCompare(transform.zoom, m_transform.zoom);
    const bool panChangedNow = transform.pan != m_transform.pan;
    if (zoomChangedNow || panChangedNow) {
        // La page va bouger sous le pointeur : le trait en cours se termine avant, là où il était (la suite,
        // placée avec la nouvelle transformation, sauterait).
        finishStroke();
    }
    m_transform = transform;
    if (m_content) {
        m_content->setScale(transform.zoom);
        m_content->setPosition(transform.pan);
    }
    if (zoomChangedNow) {
        emit zoomChanged();
    }
    if (panChangedNow) {
        emit panChanged();
    }
    if (zoomChangedNow || panChangedNow) {
        emit scrollChanged();
    }
}

void PageView::onContentSizeChanged(QSizeF oldPage)
{
    const QSizeF newPage = pageSize();
    m_lastPageSize = newPage;
    if (newPage == oldPage) {
        return;
    }
    finishStroke(); // un trait en cours reste sur sa page (comme pour un zoom)
    const ViewTransform old = m_transform;
    // Pas fitPage() ici : il demanderait lui-même un redessin du cache, et la fin de cette fonction un
    // second.
    m_cacheTimer.stop();
    apply(m_fitMode ? ui::fitPage(size(), newPage) : clampPan(m_transform, size(), newPage));
    // apply n'émet rien si ni le zoom ni le défilement n'ont bougé : la taille de la page change pourtant les
    // barres.
    if (qFuzzyCompare(old.zoom, m_transform.zoom) && old.pan == m_transform.pan) {
        emit scrollChanged();
    }
    updateCacheTextureSize();
    requestCacheRefresh();
}

void PageView::updateCacheTextureSize()
{
    const qreal dpr = window() ? window()->effectiveDevicePixelRatio() : 1.0;
    const QSizeF page = pageSize();
    // H10-9 : plafond selon l'échantillonnage du cache, jamais celui de la fenêtre ; ni flou à 100 % ni à
    // l'ajustement.
    const int maxSide = ui::cacheMaxSide(page, dpr, m_cacheSamples, ui::fitPage(size(), page).zoom);
    const QSize s = ui::cacheTextureSize(page, m_transform.zoom, dpr, maxSide);
    if (s != m_cacheTextureSize) {
        m_cacheTextureSize = s;
        emit cacheTextureSizeChanged();
    }
    // Le zoom est posé (fin de zoom, ajustement, contenu redimensionné) : la page fixe refait ses nœuds
    // rastérisés à cette échelle, jamais à chaque image d'un zoom continu.
    if (m_fixedPage) {
        m_fixedPage->setRasterScale(m_transform.zoom * dpr);
    }
}

void PageView::setCacheReady(bool ready)
{
    if (ready != m_cacheReady) {
        m_cacheReady = ready;
        emit cacheReadyChanged();
    }
}

} // namespace actcraft::ui
