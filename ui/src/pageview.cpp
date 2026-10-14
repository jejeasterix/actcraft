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
    m_content = content;
    if (m_content) {
        m_content->setTransformOrigin(QQuickItem::TopLeft);
    }
    if (m_fitMode) {
        fitPage();
    } else {
        apply(clampPan(m_transform, size(), pageSize()));
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
    m_pen->setColor(m_penColor.rgba());
    m_pen->setBaseWidth(m_penWidth);
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
    const auto sample =
        input::sampleFromEvent(event, viewTransform().viewToPage(viewPos), core::monotonicNowNs());
    if (!sample) {
        return false;
    }
    m_pipeline->push(*sample);
    return true;
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
            taken = canStartStroke(viewPos) && handlePointer(*tablet, viewPos);
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
    return !m_inputBlocked && isVisible() && contains(viewPos);
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
    if ((begin && !canStartStroke(viewPos)) || (!begin && !m_touchDrawing)) {
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

QColor PageView::penColor() const noexcept
{
    return m_penColor;
}

void PageView::setPenColor(const QColor& color)
{
    if (!color.isValid() || color == m_penColor) {
        return;
    }
    m_penColor = color;
    if (m_pen) {
        m_pen->setColor(m_penColor.rgba());
    }
    emit penChanged();
}

qreal PageView::penWidth() const noexcept
{
    return m_penWidth;
}

void PageView::setPenWidth(qreal width)
{
    const qreal clamped = std::clamp(width, kMinPenWidth, kMaxPenWidth);
    if (qFuzzyCompare(clamped, m_penWidth)) {
        return;
    }
    m_penWidth = clamped;
    if (m_pen) {
        m_pen->setBaseWidth(m_penWidth);
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
    m_fitMode = false;
    ViewTransform t = m_transform;
    const QSizeF content = pageSize() * t.zoom;
    t.pan = QPointF(panForScrollPosition(positionX, width(), content.width()),
                    panForScrollPosition(positionY, height(), content.height()));
    apply(clampPan(t, size(), pageSize()));
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
    m_fitMode = false;
    if (event->modifiers() & Qt::ControlModifier) {
        const int delta = event->angleDelta().y();
        if (delta != 0) {
            const ViewTransform t =
                clampPan(zoomAt(m_transform, event->position(), m_transform.zoom * wheelZoomFactor(delta)),
                         size(), pageSize());
            const bool changed = !qFuzzyCompare(t.zoom, m_transform.zoom);
            apply(t);
            if (changed) {
                setCacheReady(false);
                m_cacheTimer.start(); // redessin 150 ms après le dernier cran
            }
        }
    } else {
        ViewTransform t = m_transform;
        t.pan += wheelScrollDelta(event->angleDelta(), event->pixelDelta(), event->modifiers());
        apply(clampPan(t, size(), pageSize()));
    }
    event->accept();
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
