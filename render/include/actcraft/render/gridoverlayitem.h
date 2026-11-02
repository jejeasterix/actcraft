#pragma once

#include <actcraft/core/model.h>
#include <actcraft/render/fixedpageitem.h>

#include <QMetaObject>
#include <QPointer>
#include <QQuickItem>

#include <atomic>

namespace actcraft::render {

// Grille dessinée au-dessus des objets (« toujours visible », F-620), hors de la scène que le cache
// photographie : placée au-dessus de l'encre (PageArea.qml), elle couvre toute la page fixe, objets posés de
// tous les calques et trait en cours compris, sans rien découper autour d'eux (L4-O2, D17 bis). Coordonnées
// de page ; mêmes nœuds que la grille du dessous de la page fixe (createGridNode : lignes nettes de 1 px
// d'écran, lot 10, P2). Ne dessine rien si la grille de la page n'est pas au-dessus
// (core::gridDrawsAboveObjects). La masquer, la montrer ou la changer ne reconstruit que cette surcouche,
// jamais le cache (L4-T7).
class GridOverlayItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(int rebuildCount READ rebuildCount NOTIFY rebuilt) // tests
    // Amendements 1 et 2 : la page est lue par la page fixe (FixedPageItem::page()), jamais par un pointeur
    // gardé.
    Q_PROPERTY(
        actcraft::render::FixedPageItem* fixedPage READ fixedPage WRITE setFixedPage NOTIFY fixedPageChanged)

public:
    explicit GridOverlayItem(QQuickItem* parent = nullptr);
    FixedPageItem* fixedPage() const noexcept;
    // Fil principal. La grille est relue tout de suite, puis à chaque contentChanged de la page fixe
    // (changement de source compris) ; la rastérisation suit rasterScaleChanged.
    void setFixedPage(FixedPageItem* page);
    // La grille de la page affichée a pu changer (PageSettingsChanged n'atteint pas la page fixe) : relue
    // tout de suite ; la surcouche ne se refait que si la grille ou la taille de la page a changé. Appelée
    // par PageArea.qml sur area.documentController.pageContentChanged de la page affichée.
    Q_INVOKABLE void refresh();
    // Zoom de la vue (view.zoom, branché par PageArea.qml). Le dessin ne le lit pas : comme la grille du
    // dessous, celle du dessus est rastérisée (RHI) à l'échelle de la page fixe, que la vue pose en fin de
    // zoom et jamais à chaque image d'un zoom continu ; en rendu logiciel, le peintre suit la transformation.
    qreal zoom() const noexcept;
    void setZoom(qreal zoom);
    int rebuildCount() const noexcept;

signals:
    void zoomChanged();
    void rebuilt();
    void fixedPageChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData* data) override;

private:
    // Fil principal : copie la grille et la taille de la page affichée ; update() si elles ont changé.
    void readGrid();

    QPointer<FixedPageItem> m_fixedPage; // <QPointer>, <actcraft/render/fixedpageitem.h>
    QMetaObject::Connection m_contentConnection;
    QMetaObject::Connection m_scaleConnection;
    core::Grid m_grid;
    double m_width = 0;
    double m_height = 0;
    qreal m_zoom = 1;
    bool m_dirty = true;
    double m_builtScale = 0; // fil de rendu : échelle de la dernière rastérisation
    std::atomic<int> m_rebuilds{0};
};

} // namespace actcraft::render
