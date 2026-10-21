#pragma once

#include <actcraft/core/model.h>
#include <actcraft/render/fixedpageitem.h>

#include <QPointer>
#include <QQuickItem>

namespace actcraft::render {

// Grille dessinée au-dessus des objets (« toujours visible », F-620), hors de la scène que le cache
// photographie. Coordonnées de page ; lignes nettes de 1 px d'écran (lot 10, P2). Ne dessine rien si la
// grille de la page n'est pas au-dessus.
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
    void setFixedPage(FixedPageItem* page); // fil principal
    // La grille de la page affichée a pu changer (PageSettingsChanged n'atteint pas la page fixe) : relue au
    // prochain rendu. Appelée par PageArea.qml sur area.documentController.pageContentChanged de la page
    // affichée.
    Q_INVOKABLE void refresh();
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
    QPointer<FixedPageItem> m_fixedPage; // <QPointer>, <actcraft/render/fixedpageitem.h>
    core::Grid m_grid;
    double m_width = 0;
    double m_height = 0;
    qreal m_zoom = 1;
    bool m_dirty = true;
    int m_rebuilds = 0;
};

} // namespace actcraft::render
