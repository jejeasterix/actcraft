#include "objectnodes.h"

#include <actcraft/core/pagesettings.h>
#include <actcraft/render/gridoverlayitem.h>
#include <actcraft/render/paintedrenderer.h>

#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSGTransformNode>

#include <cstddef>
#include <utility>

namespace actcraft::render {

namespace {

bool sameLevel(const core::GridLevel& a, const core::GridLevel& b) noexcept
{
    return a.visible == b.visible && a.snap == b.snap && a.start.x == b.start.x && a.start.y == b.start.y &&
           a.spacingX == b.spacingX && a.spacingY == b.spacingY && a.color == b.color &&
           a.pattern == b.pattern && a.angle == b.angle && a.line == b.line &&
           a.linesAlongX == b.linesAlongX && a.linesAlongY == b.linesAlongY;
}

// Tout ce que le peintre de grille peut lire (les clés inconnues ne se dessinent pas).
bool sameGrid(const core::Grid& a, const core::Grid& b) noexcept
{
    if (a.visible != b.visible || a.snapEnabled != b.snapEnabled || a.drawnAbove != b.drawnAbove ||
        a.previewZoom != b.previewZoom || a.baseZoom != b.baseZoom || a.zoomIncrement != b.zoomIncrement ||
        a.levels.size() != b.levels.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.levels.size(); ++i) {
        if (!sameLevel(a.levels[i], b.levels[i])) {
            return false;
        }
    }
    return true;
}

} // namespace

GridOverlayItem::GridOverlayItem(QQuickItem* parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents);
}

FixedPageItem* GridOverlayItem::fixedPage() const noexcept
{
    return m_fixedPage.data();
}

void GridOverlayItem::setFixedPage(FixedPageItem* page)
{
    if (m_fixedPage == page) {
        return;
    }
    disconnect(m_contentConnection);
    disconnect(m_scaleConnection);
    m_fixedPage = page;
    if (page) {
        // Changement de source (setSource, setPage) et changements de contenu : la grille est comparée, la
        // surcouche ne se refait que si elle a changé (un trait ajouté ne la refait pas).
        m_contentConnection = connect(page, &FixedPageItem::contentChanged, this, &GridOverlayItem::readGrid);
        m_scaleConnection = connect(page, &FixedPageItem::rasterScaleChanged, this, [this] { update(); });
    }
    readGrid();
    emit fixedPageChanged();
}

void GridOverlayItem::refresh()
{
    readGrid();
}

void GridOverlayItem::readGrid()
{
    core::Grid grid;
    double width = 0;
    double height = 0;
    if (m_fixedPage) {
        const core::Page& page = m_fixedPage->page();
        grid = page.grid;
        width = page.width;
        height = page.height;
    }
    if (sameGrid(grid, m_grid) && width == m_width && height == m_height) {
        return;
    }
    m_grid = std::move(grid);
    m_width = width;
    m_height = height;
    m_dirty = true;
    update();
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
    return m_rebuilds.load();
}

QSGNode* GridOverlayItem::updatePaintNode(QSGNode* old, UpdatePaintNodeData*)
{
    const bool software = window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software;
    const bool forceRasterized = m_fixedPage && m_fixedPage->forceRasterized();
    // Échelle de la page fixe (posée par la vue en fin de zoom) : la grille du dessus se rastérise quand et
    // comme celle du dessous (FixedPageItem::rescaleRasterNodes).
    const double scale = m_fixedPage ? m_fixedPage->rasterScale() : 1.0;
    const NodeContexts contexts = nodeContexts(window(), software, nullptr, scale, forceRasterized);
    const bool rasterized = !contexts.painted.software;
    if (old && !m_dirty && !(rasterized && scale != m_builtScale)) {
        return old;
    }
    // Logiciel : racine de transformation (identité), comme la page fixe : sous une racine QSGNode nue déjà
    // rendue, le moteur logiciel de Qt 6.12 peint les nœuds ajoutés sans la transformation de l'élément.
    QSGNode* root = old ? old : (software ? new QSGTransformNode : new QSGNode);
    clearChildren(root);
    if (core::gridDrawsAboveObjects(m_grid)) {
        // Page sans objet ni fond : createGridNode ne dessine que sa grille, sur toute la page.
        core::Page gridPage;
        gridPage.width = m_width;
        gridPage.height = m_height;
        gridPage.grid = m_grid;
        if (QSGNode* grid = createGridNode(gridPage, contexts.painted)) {
            root->appendChildNode(grid);
        }
    }
    m_dirty = false;
    m_builtScale = scale;
    ++m_rebuilds;
    // Fil de rendu : la notification part sur le fil de l'élément (liaisons QML).
    QMetaObject::invokeMethod(this, &GridOverlayItem::rebuilt, Qt::QueuedConnection);
    return root;
}

} // namespace actcraft::render
