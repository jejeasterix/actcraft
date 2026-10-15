#include "penadapters.h"

#include <actcraft/core/clock.h>
#include <actcraft/input/pipeline.h>
#include <actcraft/input/qtevents.h>
#include <actcraft/tools/pentool.h>
#include <actcraft/ui/pageview.h>

#include <QMouseEvent>
#include <QQuickWindow>
#include <QTabletEvent>
#include <QTouchEvent>
#include <QWheelEvent>

#include <algorithm>

namespace actcraft::ui {

namespace {

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

} // namespace

PageView::PageView(QQuickItem* parent) : QQuickItem(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);
    m_cacheTimer.setSingleShot(true);
    m_cacheTimer.setInterval(kCacheRefreshDelayMs);
    connect(&m_cacheTimer, &QTimer::timeout, this, [this] {
        updateCacheTextureSize();
        requestCacheRefresh();
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
    m_content = content;
    if (m_content) {
        m_content->setTransformOrigin(QQuickItem::TopLeft);
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
    emit cacheEnabledChanged();
    requestCacheRefresh();
}

QSize PageView::cacheTextureSize() const noexcept
{
    return m_cacheTextureSize;
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
    if (!m_cacheEnabled) {
        m_refreshInFlight = false;
        m_refreshAgain = false;
        setCacheReady(true);
        emit cacheUpdated();
        return;
    }
    setCacheReady(false);
    if (m_refreshInFlight) {
        m_refreshAgain = true; // la fin du redessin en cours ne suffira pas
    }
    m_refreshInFlight = true;
    emit cacheRefreshRequested(); // toujours tout de suite (décision 10)
}

void PageView::notifyCacheUpdated()
{
    if (m_refreshAgain) {
        // Ce redessin a pu commencer avant la dernière demande : un de plus garantit l'état final.
        m_refreshAgain = false;
        emit cacheRefreshRequested();
        return;
    }
    m_refreshInFlight = false;
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
    disconnect(m_readyConnection);
    m_fixedPage = page;
    if (m_fixedPage) {
        // Page prête : premier dessin du cache.
        m_readyConnection = connect(m_fixedPage, &render::FixedPageItem::readyChanged, this, [this] {
            if (m_fixedPage && m_fixedPage->isReady()) {
                requestCacheRefresh();
            }
        });
    }
    rebuildPen();
    emit fixedPageChanged();
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

void PageView::onStrokeFrozen()
{
    emit strokeFrozen();
    requestCacheRefresh();
}

void PageView::rebuildPen()
{
    m_pipeline.reset();
    m_pen.reset();
    m_documentSink.reset();
    m_inkSurface.reset();
    if (!m_fixedPage || !m_ink) {
        return;
    }
    m_inkSurface = std::make_unique<InkSurfaceAdapter>(*m_ink);
    m_documentSink = std::make_unique<FixedPageSink>(*m_fixedPage, *this);
    m_pen = std::make_unique<tools::PenTool>(*m_inkSurface, *m_documentSink);
    m_pen->setColor(m_inkColor.rgba());
    m_pen->setBaseWidth(m_inkWidth);
    m_pipeline = std::make_unique<input::Pipeline>(*m_pen);
    m_pipeline->setProbe(m_probe);
    m_ink->setProbe(m_probe);
}

bool PageView::handlePointer(const QEvent& event, QPointF viewPos)
{
    if (!m_pipeline) {
        return false;
    }
    const auto& pointer = static_cast<const QPointerEvent&>(event);
    if (!acceptsDevice(pointer.pointingDevice())) {
        return false;
    }
    const QPointF pagePos = viewTransform().viewToPage(viewPos);
    const auto sample = input::sampleFromEvent(event, pagePos, core::monotonicNowNs());
    if (!sample) {
        return false;
    }
    m_lastPagePoint = pagePos;
    m_pipeline->push(*sample);
    return true;
}

void PageView::finishStroke()
{
    m_touchDrawing = false; // la suite du contact du doigt ne dessine plus
    // Stylet : m_stylusDrawing reste vrai, le contact reste pris jusqu'au relâché (rien n'en passe à
    // l'interface) ; ses points n'ajoutent rien, le stylo ne dessine plus. Souris : idem par la saisie.
    if (!m_pipeline || !m_pen || !m_pen->isDrawing()) {
        return;
    }
    input::Sample release;
    release.x = m_lastPagePoint.x(); // au dernier point : le relâché n'ajoute aucun point
    release.y = m_lastPagePoint.y();
    release.timestampNs = core::monotonicNowNs();
    release.phase = input::Phase::Release;
    m_pipeline->push(release);
}

void PageView::mousePressEvent(QMouseEvent* event)
{
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
        }
        m_filteredWindow = value.window;
        if (m_filteredWindow) {
            m_filteredWindow->installEventFilter(this);
        }
    }
}

bool PageView::eventFilter(QObject* watched, QEvent* event)
{
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
            if (canStartStroke(viewPos) && acceptsDevice(tablet->pointingDevice()) && !coveredAt(viewPos)) {
                // Un seul trait à la fois : le stylet termine d'abord celui du doigt (PenTool mêlerait les
                // points des deux).
                finishTouchStroke();
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
    const QQuickItem* parent = parentItem();
    if (!parent) {
        return false;
    }
    // Ordre de Qt Quick : z croissant, puis ordre des enfants à z égal.
    const QPointF scenePos = mapToScene(viewPos);
    const QList<QQuickItem*> siblings = parent->childItems();
    const qsizetype self = siblings.indexOf(this);
    for (qsizetype i = 0; i < siblings.size(); ++i) {
        const QQuickItem* other = siblings.at(i);
        const bool above = other->z() > z() || (!(other->z() < z()) && i > self);
        if (above && takesPressAt(other, scenePos)) {
            return true;
        }
    }
    return false;
}

void PageView::touchEvent(QTouchEvent* event)
{
    if (event->type() == QEvent::TouchCancel) {
        finishTouchStroke();
        event->accept();
        return;
    }
    // L'événement remis à la vue ne porte que ses points (saisis par elle ou appuyés sur elle) : un deuxième
    // doigt posé sur la vue y figure avec le premier ; posé ailleurs (palette), il n'arrête rien.
    if (event->pointCount() != 1) {
        finishTouchStroke(); // un deuxième doigt arrête le trait du premier (gestes : lot 3)
        event->ignore();
        return;
    }
    const QPointF viewPos = event->points().constFirst().position();
    const bool begin = event->type() == QEvent::TouchBegin;
    // Pendant un trait du stylet, un doigt (ou la paume) ne commence rien : le trait du stylet continue.
    if ((begin && (m_stylusDrawing || !canStartStroke(viewPos))) || (!begin && !m_touchDrawing)) {
        event->ignore();
        return;
    }
    m_lastTouchViewPos = viewPos;
    const bool handled = handlePointer(*event, viewPos);
    if (begin) {
        m_touchDrawing = handled;
    } else if (event->type() == QEvent::TouchEnd) {
        m_touchDrawing = false;
    }
    event->setAccepted(handled);
}

void PageView::touchUngrabEvent()
{
    finishTouchStroke(); // saisie volée (futur geste, Flickable) : le trait du doigt se termine proprement
    QQuickItem::touchUngrabEvent();
}

void PageView::finishTouchStroke()
{
    if (!m_touchDrawing) {
        return;
    }
    m_touchDrawing = false;
    if (!m_pipeline) {
        return;
    }
    const QPointF page = viewTransform().viewToPage(m_lastTouchViewPos);
    input::Sample release;
    release.x = page.x();
    release.y = page.y();
    release.timestampNs = core::monotonicNowNs();
    release.device = input::DeviceKind::Touch;
    release.phase = input::Phase::Release;
    m_pipeline->push(release);
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
        const ViewTransform t =
            clampPan(zoomAt(m_transform, event->position(), m_transform.zoom * wheelZoomFactor(delta)),
                     size(), pageSize());
        const bool changed = !qFuzzyCompare(t.zoom, m_transform.zoom);
        if (!changed && samePan(t.pan, m_transform.pan)) {
            return;
        }
        m_fitMode = false;
        apply(t);
        if (changed) {
            setCacheReady(false);
            m_cacheTimer.start(); // redessin 150 ms après le dernier cran
        }
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

void PageView::updateCacheTextureSize()
{
    const qreal dpr = window() ? window()->effectiveDevicePixelRatio() : 1.0;
    const QSize s = ui::cacheTextureSize(pageSize(), m_transform.zoom, dpr);
    if (s != m_cacheTextureSize) {
        m_cacheTextureSize = s;
        emit cacheTextureSizeChanged();
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
