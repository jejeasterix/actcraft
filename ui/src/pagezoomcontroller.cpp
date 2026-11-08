#include "pagezoomcontroller.h"

#include <actcraft/core/clock.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/viewtransform.h>

namespace actcraft::ui {

// Les bornes de features::PageZoom sont celles de la vue (features ne dépend pas d'ui).
static_assert(features::PageZoom::kMinZoom == kMinZoom);
static_assert(features::PageZoom::kMaxZoom == kMaxZoom);

namespace {
constexpr int kTickMs = 16;
}

PageZoomController::PageZoomController(QObject* parent) : QObject(parent)
{
    m_timer.setInterval(kTickMs);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, [this] { tick(); });
}

bool PageZoomController::active() const noexcept
{
    return m_active;
}

void PageZoomController::setActive(bool active)
{
    if (m_active == active) {
        return;
    }
    if (!active) {
        release();
    }
    m_active = active;
    emit activeChanged();
}

PageView* PageZoomController::view() const noexcept
{
    return m_view;
}

void PageZoomController::setView(PageView* view)
{
    if (m_view == view) {
        return;
    }
    release();
    m_view = view;
    emit viewChanged();
}

bool PageZoomController::panning() const noexcept
{
    return m_panning;
}

void PageZoomController::toggle()
{
    setActive(!m_active);
}

void PageZoomController::press(QPointF scenePoint, bool right)
{
    if (!m_active || !m_view || m_zoom.pressed()) {
        return;
    }
    const QPointF point = m_view->mapFromScene(scenePoint);
    const std::int64_t now = core::monotonicNowNs();
    if (m_zoom.isDoubleClick(point, now)) {
        m_view->fitPage();
        return;
    }
    m_zoom.setZoom(m_view->zoom());
    m_zoom.press(point, right ? features::PageZoom::Direction::Out : features::PageZoom::Direction::In, now);
    m_timer.start();
}

void PageZoomController::move(QPointF scenePoint)
{
    if (!m_view || !m_zoom.pressed()) {
        return;
    }
    const std::int64_t now = core::monotonicNowNs();
    const bool wasPanning = m_zoom.panning();
    if (!m_zoom.move(m_view->mapFromScene(scenePoint), now)) {
        return;
    }
    if (!wasPanning) {
        // L'appui devient un panoramique : le zoom déjà fait est rendu (PageZoom le fige à son départ).
        m_timer.stop();
        m_view->zoomAround(m_zoom.anchor(), m_zoom.zoomAt(now));
        m_panning = true;
        emit panningChanged();
    }
    m_view->setPan(m_view->pan() + m_zoom.panDelta());
}

void PageZoomController::release()
{
    m_timer.stop();
    if (!m_zoom.pressed()) {
        return;
    }
    const bool wasPanning = m_zoom.panning();
    m_zoom.release(core::monotonicNowNs());
    if (m_view && !wasPanning) {
        // Dernier zoom : la vue redessine le cache une fois, après son délai (aucune demande d'ici).
        m_view->zoomAround(m_zoom.anchor(), m_zoom.zoomAt(0));
    }
    if (m_panning) {
        m_panning = false;
        emit panningChanged();
    }
}

void PageZoomController::tick()
{
    if (!m_view || !m_zoom.pressed() || m_zoom.panning()) {
        return;
    }
    m_view->zoomAround(m_zoom.anchor(), m_zoom.zoomAt(core::monotonicNowNs()));
}

} // namespace actcraft::ui
