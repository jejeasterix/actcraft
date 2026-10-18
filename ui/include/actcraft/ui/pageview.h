#pragma once

#include <actcraft/core/inkprobe.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/render/pendinglayeritem.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/viewtransform.h>

#include <QColor>
#include <QList>
#include <QPointer>
#include <QQuickItem>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include <cstdint>
#include <memory>

class QInputDevice;

namespace actcraft::input {
class Pipeline;
}
namespace actcraft::tools {
class PenTool;
}

namespace actcraft::ui {

class InkSurfaceAdapter;
class FixedPageSink;
class ControllerSink;

// Épaisseur du stylo, pixels de page (F-137 : réglette 1 à 100 ; F-220 : 4 par défaut).
inline constexpr qreal kDefaultInkWidth = 4.0;
inline constexpr qreal kMinInkWidth = 1.0;
inline constexpr qreal kMaxInkWidth = 100.0;

// Vue de la page (décision 14) : zoom, défilement, politique du cache. Transforme l'élément `content`.
class PageView : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QQuickItem* content READ content WRITE setContent NOTIFY contentChanged)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(QPointF pan READ pan WRITE setPan NOTIFY panChanged)
    Q_PROPERTY(bool cacheEnabled READ cacheEnabled WRITE setCacheEnabled NOTIFY cacheEnabledChanged)
    Q_PROPERTY(QSize cacheTextureSize READ cacheTextureSize NOTIFY cacheTextureSizeChanged)
    Q_PROPERTY(bool cacheReady READ cacheReady NOTIFY cacheReadyChanged)
    Q_PROPERTY(
        actcraft::render::FixedPageItem* fixedPage READ fixedPage WRITE setFixedPage NOTIFY fixedPageChanged)
    Q_PROPERTY(actcraft::render::InkItem* ink READ ink WRITE setInk NOTIFY inkChanged)
    // Lot 10 : couche d'attente au-dessus du cache. Branchée et cache actif : la page fixe est en différé.
    Q_PROPERTY(actcraft::render::PendingLayerItem* pendingLayer READ pendingLayer WRITE setPendingLayer NOTIFY
                   pendingLayerChanged)
    Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                   setDocumentController NOTIFY documentControllerChanged)
    Q_PROPERTY(QColor inkColor READ inkColor WRITE setInkColor NOTIFY penChanged)
    Q_PROPERTY(qreal inkWidth READ inkWidth WRITE setInkWidth NOTIFY penChanged)
    Q_PROPERTY(bool inputBlocked READ inputBlocked WRITE setInputBlocked NOTIFY inputBlockedChanged)
    Q_PROPERTY(qreal scrollSizeX READ scrollSizeX NOTIFY scrollChanged)
    Q_PROPERTY(qreal scrollPositionX READ scrollPositionX NOTIFY scrollChanged)
    Q_PROPERTY(qreal scrollSizeY READ scrollSizeY NOTIFY scrollChanged)
    Q_PROPERTY(qreal scrollPositionY READ scrollPositionY NOTIFY scrollChanged)

public:
    explicit PageView(QQuickItem* parent = nullptr);
    ~PageView() override;

    render::FixedPageItem* fixedPage() const noexcept;
    void setFixedPage(render::FixedPageItem* page);
    render::InkItem* ink() const noexcept;
    void setInk(render::InkItem* ink);
    // Lot 10 (DA11) : couche d'attente branchée, qui regarde la page fixe de la vue, et cache actif, un objet
    // ajouté en fin de page attend au-dessus du cache ; il y est versé après kCacheFoldIdleMs sans entrée qui
    // agit (un survol ne compte pas), dès kCacheFoldMaxPending objets en attente, et à tout redessin du cache
    // (requestCacheRefresh : zoom, page, reconstruction). Sans couche (ou couche détruite, ou couche d'une
    // autre page fixe), ou sans cache : comportement du lot 2. Une page fixe remplacée quitte le différé.
    render::PendingLayerItem* pendingLayer() const noexcept;
    void setPendingLayer(render::PendingLayerItem* layer);
    bool foldTimerActive() const noexcept; // crochet de test
    DocumentController* documentController() const noexcept;
    // Posé : le stylo verse ses traits au document par la pile du contrôleur (ControllerSink), qui doit
    // montrer la même page fixe que la vue. Sinon : dans la page interne de la page fixe (banc, tests).
    void setDocumentController(DocumentController* controller);
    // Banc : sonde de latence (DA4) et numérotation des échantillons depuis `next`.
    void setInkProbe(core::InkProbe* probe);
    void resetInputSequence(std::uint64_t next = 0);
    // Appelé par l'adaptateur du document quand un trait est figé : émet strokeFrozen, rien de plus. Le
    // redessin du cache vient de FixedPageItem::contentChanged, une fois par trait.
    void onStrokeFrozen();

    QQuickItem* content() const noexcept;
    void setContent(QQuickItem* content);
    qreal zoom() const noexcept;
    // Zoom imposé (banc, programme) : ancré au centre de la vue, quitte « Ajuster », cache redessiné tout de
    // suite.
    void setZoom(qreal zoom);
    QPointF pan() const noexcept;
    void setPan(QPointF pan);
    bool cacheEnabled() const noexcept;
    void setCacheEnabled(bool enabled);
    QSize cacheTextureSize() const noexcept;
    // Vrai quand ce qui est affiché (texture du cache et couche d'attente) correspond à la page et au zoom
    // courants (toujours vrai sans cache) ; un objet qui attend son versement ne le rend pas faux.
    bool cacheReady() const noexcept;
    const ViewTransform& viewTransform() const noexcept;
    QColor inkColor() const noexcept;
    // Couleur invalide ignorée.
    void setInkColor(const QColor& color);
    qreal inkWidth() const noexcept;
    // Bornée à [kMinInkWidth, kMaxInkWidth].
    void setInkWidth(qreal width);
    bool inputBlocked() const noexcept;
    // Vrai tant qu'une fenêtre surgissante est ouverte (menu, dialogue) : aucun trait ne commence ; un trait
    // commencé se termine normalement.
    void setInputBlocked(bool blocked);
    qreal scrollSizeX() const;
    qreal scrollPositionX() const;
    qreal scrollSizeY() const;
    qreal scrollPositionY() const;
    // Barres de défilement (F-022) : positions en fraction de l'étendue ; quitte « Ajuster ».
    Q_INVOKABLE void scrollTo(qreal positionX, qreal positionY);

    Q_INVOKABLE void fitPage();
    // Demande un redessin du cache (QML : cache.scheduleUpdate()) ; sans cache, « prêt » tout de suite.
    // Lot 10 : verse d'abord les objets en attente (FixedPageItem::foldPending), dans le même tour de boucle
    // que la demande : la synchronisation suivante photographie une scène qui les contient pendant que la
    // couche d'attente ne les dessine plus. Un objet inséré après la demande attend le versement suivant.
    // La demande part toujours tout de suite (un trait figé n'attend pas, décision 10). Si elle arrive
    // pendant un redessin, la fin de celui-ci en relance un autre : cacheReady ne redevient vrai qu'après un
    // redessin commencé après la dernière demande. Une demande perdue (QML pas encore branché) est rattrapée
    // par la suivante.
    Q_INVOKABLE void requestCacheRefresh();
    // QML : appelé sur ShaderEffectSource.scheduledUpdateCompleted.
    Q_INVOKABLE void notifyCacheUpdated();
    Q_INVOKABLE QPointF pageToScene(QPointF pagePoint) const;
    // Termine le trait en cours (stylet, doigt ou souris) à son dernier point et le verse à la page affichée
    // ; la suite du même contact ne dessine plus. Appelé avant tout changement de page
    // (DocumentController::pageAboutToChange, branché dans Main.qml) et, par la vue elle-même, avant tout
    // changement de zoom ou de défilement : un trait n'est jamais perdu ni versé à une autre page.
    Q_INVOKABLE void finishStroke();
    // Pendant un scénario du banc, seuls ces périphériques sont écoutés ; liste vide = tous.
    void setExclusiveDevices(QList<const QInputDevice*> devices);

signals:
    void contentChanged();
    void zoomChanged();
    void panChanged();
    void cacheEnabledChanged();
    void cacheTextureSizeChanged();
    void cacheReadyChanged();
    void cacheRefreshRequested();
    void cacheUpdated();
    void fixedPageChanged();
    void inkChanged();
    void pendingLayerChanged();
    void documentControllerChanged();
    void strokeFrozen();
    void penChanged();
    void inputBlockedChanged();
    void scrollChanged();

protected:
    void wheelEvent(QWheelEvent* event) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    bool acceptsDevice(const QInputDevice* device) const;
    bool event(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void touchEvent(QTouchEvent* event) override;
    void touchUngrabEvent() override;
    void itemChange(ItemChange change, const ItemChangeData& value) override;

private:
    QSizeF pageSize() const;
    void apply(const ViewTransform& transform);
    void updateCacheTextureSize();
    void onContentSizeChanged(QSizeF oldPage);
    void setCacheReady(bool ready);
    void rebuildPen();
    bool handlePointer(const QEvent& event, QPointF viewPos);
    // Un trait peut commencer ici : vue visible, aucune fenêtre surgissante, point sur la vue.
    bool canStartStroke(QPointF viewPos) const;
    // Un élément posé sur la vue (empilé au-dessus d'elle chez son parent : barres de défilement) prendrait
    // la souris ou le doigt à ce point. Qt Quick le leur livre avant la vue ; le stylet, capté par le filtre
    // de la fenêtre, doit leur être laissé de même. Consulté à l'appui seulement, jamais par point.
    bool coveredAt(QPointF viewPos) const;
    // Termine le trait d'un doigt (deuxième doigt, annulation) par un relâché au dernier point connu.
    void finishTouchStroke();
    // Lot 10 : différé posé sur la page fixe quand le cache est actif et une couche d'attente branchée.
    void updateDeferral();
    void onPendingChanged();
    // Une entrée qui agit relance la minuterie du versement, si elle court.
    void postponeFold();

    render::FixedPageItem* m_fixedPage = nullptr;
    render::InkItem* m_ink = nullptr;
    core::InkProbe* m_probe = nullptr;
    std::unique_ptr<InkSurfaceAdapter> m_inkSurface;
    std::unique_ptr<FixedPageSink> m_documentSink;
    QPointer<DocumentController> m_controller;
    std::unique_ptr<ControllerSink> m_controllerSink;
    QMetaObject::Connection m_controllerConnection;
    std::unique_ptr<tools::PenTool> m_pen;
    std::unique_ptr<input::Pipeline> m_pipeline;
    QMetaObject::Connection m_readyConnection;
    QMetaObject::Connection m_contentConnection; // contentChanged de la page fixe : redessin du cache
    QMetaObject::Connection m_pendingConnection; // pendingChanged de la page fixe : minuterie, plafond
    QPointer<render::PendingLayerItem> m_pendingLayer;
    QMetaObject::Connection m_layerPageConnection;      // la couche change de page fixe : différé revu
    QMetaObject::Connection m_layerDestroyedConnection; // la couche détruite : différé levé
    QPointer<QQuickWindow> m_filteredWindow; // variante B : le stylet est capté par un filtre sur la fenêtre

    QQuickItem* m_content = nullptr;
    QMetaObject::Connection m_contentWidthConnection;
    QMetaObject::Connection m_contentHeightConnection;
    QSizeF m_lastPageSize;
    ViewTransform m_transform;
    bool m_fitMode = true;
    bool m_cacheEnabled = true;
    bool m_cacheReady = false;
    bool m_refreshInFlight = false; // un redessin du cache est demandé et pas encore terminé
    bool m_refreshAgain = false;    // une demande est arrivée pendant ce redessin : en refaire un à sa fin
    QSize m_cacheTextureSize;
    QTimer m_cacheTimer;
    QTimer m_foldTimer; // lot 10 : versement après kCacheFoldIdleMs sans entrée qui agit
    QList<const QInputDevice*> m_exclusive;
    QColor m_inkColor = Qt::black;
    qreal m_inkWidth = kDefaultInkWidth;
    bool m_inputBlocked = false;
    bool m_stylusDrawing = false; // un trait du stylet, commencé sur la vue, est en cours
    bool m_touchDrawing = false;  // un trait d'un doigt est en cours
    QPointF m_lastTouchViewPos;
    QPointF m_lastPagePoint; // dernier point transmis au stylo, en coordonnées de page
};

} // namespace actcraft::ui
