#include <actcraft/render/gridoverlayitem.h>

namespace actcraft::render {

// Talon L4-T1 : rempli par L4-T7.
GridOverlayItem::GridOverlayItem(QQuickItem* parent) : QQuickItem(parent) {}

FixedPageItem* GridOverlayItem::fixedPage() const noexcept
{
    return m_fixedPage.data();
}

void GridOverlayItem::setFixedPage(FixedPageItem* page)
{
    if (m_fixedPage == page) {
        return;
    }
    m_fixedPage = page;
    emit fixedPageChanged();
}

void GridOverlayItem::refresh()
{
    m_dirty = true;
    static_cast<void>(m_grid);
    static_cast<void>(m_width);
    static_cast<void>(m_height);
}

qreal GridOverlayItem::zoom() const noexcept
{
    return m_zoom;
}

void GridOverlayItem::setZoom(qreal zoom)
{
    if (m_zoom == zoom) {
        return;
    }
    m_zoom = zoom;
    emit zoomChanged();
}

int GridOverlayItem::rebuildCount() const noexcept
{
    return m_rebuilds;
}

QSGNode* GridOverlayItem::updatePaintNode(QSGNode* old, UpdatePaintNodeData*)
{
    return old;
}

} // namespace actcraft::render
