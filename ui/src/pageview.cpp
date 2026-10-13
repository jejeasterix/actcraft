#include <actcraft/ui/pageview.h>

#include <QQuickWindow>
#include <QWheelEvent>

namespace actcraft::ui {

PageView::PageView(QQuickItem* parent) : QQuickItem(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    m_cacheTimer.setSingleShot(true);
    m_cacheTimer.setInterval(kCacheRefreshDelayMs);
    connect(&m_cacheTimer, &QTimer::timeout, this, [this] {
        updateCacheTextureSize();
        requestCacheRefresh();
    });
}

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
