#pragma once

#include <actcraft/core/inkprobe.h>
#include <actcraft/input/gesture.h>
#include <actcraft/input/routing.h>
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

#include <array>
#include <cstdint>
#include <functional>
#include <memory>

class QInputDevice;

namespace actcraft::input {
class Pipeline;
}
namespace actcraft::tools {
class PenTool;
class Tool;
} // namespace actcraft::tools

namespace actcraft::ui {

class InkSurfaceAdapter;
class FixedPageSink;
class ControllerSink;
class ToolHost;
class ToolDispatch;

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
    // Échantillonnage de la texture du cache (H10-9) : seule source, liée à ShaderEffectSource.samples
    // (PageArea.qml). Fixé à l'attache à une fenêtre : kCacheMsaaSamples en RHI, 1 en rendu logiciel ; le
    // plafond de la texture en dépend. En RHI, 1 ferait suivre l'échantillonnage de la fenêtre au cache
    // (QSGRhiLayer : effectiveSamples <= 1 → msaaSampleCount()) : PageView ne le pose jamais.
    Q_PROPERTY(int cacheSamples READ cacheSamples WRITE setCacheSamples NOTIFY cacheSamplesChanged)
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
    // Socle des lots 3 et 7 (L0-S7) : chemin d'entrée figé. toolhost.h inclut pageview.h : inclusion
    // circulaire évitée.
    Q_MOC_INCLUDE(<actcraft / ui / toolhost.h>)
    Q_PROPERTY(actcraft::ui::ToolHost* tools READ tools WRITE setTools NOTIFY toolsChanged)
    Q_PROPERTY(int cacheRefreshCount READ cacheRefreshCount NOTIFY cacheUpdated)
    Q_PROPERTY(bool gestureActive READ gestureActive NOTIFY gestureActiveChanged)

public:
    using SampleFilter = std::function<input::Sample(const input::Sample&)>;

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
    int cacheSamples() const noexcept;
    void setCacheSamples(int samples); // borné à 1 au moins
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

    // Chemin d'entrée (D-S1) : événement → Sample → PointerRouter (couvertures) → filtre d'échantillons (voie
    // Tool) → Pipeline → outil actif.
    ToolHost* tools() const noexcept;
    // Posé : les échantillons routés vers l'outil vont à ToolHost::activeTool(utilisateur du contact) ;
    // sinon, ou s'il rend nullptr, au stylo interne (lots 1 et 2, banc).
    void setTools(ToolHost* tools);
    actcraft::tools::Tool* currentTool() const; // outil qui reçoit la voie Tool maintenant
    int
    cacheRefreshCount() const noexcept;  // redessins du cache terminés depuis la création (crochet S6, H10-1)
    bool gestureActive() const noexcept; // un geste à deux doigts est en cours (L3-T13 ; faux au socle)
    input::PointerRouter& router() noexcept; // tests
    // Surcouche (lot 7) qui couvre la page là où elle est visible : à l'appui sous elle, ni stylet, ni doigt,
    // ni souris ne vont à l'outil (Route::Cover) ; l'événement continue vers ses gestionnaires. Oubliée à sa
    // destruction.
    Q_INVOKABLE void addInputCover(QQuickItem* cover);
    Q_INVOKABLE void removeInputCover(QQuickItem* cover);
    // Guides de tracé (lot 7) : appliqué à chaque échantillon de la voie Tool, avant le Pipeline, relâchés
    // synthétisés compris (à partir du dernier point brut). Vide : aucun. Fil principal.
    void setSampleFilter(SampleFilter filter);
    // Zoom ancré : le point de page sous viewPoint ne bouge pas ; quitte « Ajuster » ; cache comme la molette
    // (redessin kCacheRefreshDelayMs après le dernier appel). Pincement (L3-T13) et zoom de page (L7-T5).
    Q_INVOKABLE void zoomAround(QPointF viewPoint, qreal zoom);
    Q_INVOKABLE QPointF sceneToPage(QPointF scenePoint) const; // inverse de pageToScene

signals:
    void contentChanged();
    void zoomChanged();
    void panChanged();
    void cacheEnabledChanged();
    void cacheTextureSizeChanged();
    void cacheSamplesChanged();
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
    void toolsChanged();
    void gestureActiveChanged();
    void contextMenuRequested(QPointF viewPos); // clic droit non couvert sur la vue (F-202)

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
    // Termine le trait d'un doigt (deuxième doigt, annulation) par un relâché au dernier point brut du doigt,
    // filtré comme les autres, et rend son contact au routeur.
    void finishTouchStroke();
    // Voie Tool : filtre d'échantillons s'il est posé, puis Pipeline (numérotation, sonde, doublons).
    void pushToTool(const input::Sample& sample);
    // Une couverture détruite est oubliée.
    void forgetDeadCovers();
    // Lot 10 : différé posé sur la page fixe quand le cache est actif et une couche d'attente branchée.
    void updateDeferral();
    void onPendingChanged();
    // Une entrée qui agit relance la minuterie du versement, si elle court.
    void postponeFold();
    // L3-T13 : un contact du geste à deux doigts (pointer 0 : place libre ; un doigt vaut identifiant + 1).
    struct GestureContact
    {
        std::uint64_t pointer = 0;
        QPointF viewPos;
    };
    // endToolContacts du routeur : le doigt qui écrit passe au geste (trait abandonné, H3-17) ou cède la main
    // au stylet ou à la souris (trait terminé là où il était, M4 b).
    void endToolContacts(const input::Decision& decision);
    void
    terminateTouchStroke(); // relâché au dernier point brut du doigt, puis cancel() si l'outil reste occupé
    void abandonTouchStroke(); // Tool::abandon : rien n'est versé
    bool onGestureSample(const input::Sample& sample, QPointF viewPos);
    bool addGestureContact(std::uint64_t pointer, QPointF viewPos); // faux : le geste a ses deux contacts
    void applyGesture();
    void endGesture(); // annulation, saisie volée : contacts du geste oubliés, au routeur aussi
    // M3 : les demandes de redessin suivantes de ce tour sont couvertes par celle qui vient de partir.
    void holdRequestsOfThisTurn();

    render::FixedPageItem* m_fixedPage = nullptr;
    render::InkItem* m_ink = nullptr;
    core::InkProbe* m_probe = nullptr;
    std::unique_ptr<InkSurfaceAdapter> m_inkSurface;
    std::unique_ptr<FixedPageSink> m_documentSink;
    QPointer<DocumentController> m_controller;
    std::unique_ptr<ControllerSink> m_controllerSink;
    QMetaObject::Connection m_controllerConnection;
    std::unique_ptr<actcraft::tools::PenTool> m_pen; // qualifié : tools() est un membre
    std::unique_ptr<ToolDispatch> m_dispatch;        // fin du Pipeline : currentTool()
    std::unique_ptr<input::Pipeline> m_pipeline;
    input::PointerRouter m_router;
    QList<QPointer<QQuickItem>> m_covers;
    SampleFilter m_filter;
    QPointer<ToolHost> m_tools;
    int m_cacheRefreshCount = 0;
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
    int m_cacheSamples = 1;
    QTimer m_cacheTimer;
    QTimer m_foldTimer; // lot 10 : versement après kCacheFoldIdleMs sans entrée qui agit
    QList<const QInputDevice*> m_exclusive;
    QColor m_inkColor = Qt::black;
    qreal m_inkWidth = kDefaultInkWidth;
    bool m_inputBlocked = false;
    bool m_stylusDrawing = false;     // un trait du stylet, commencé sur la vue, est en cours
    bool m_touchDrawing = false;      // un trait d'un doigt est en cours
    std::uint64_t m_touchPointer = 0; // contact du doigt qui trace (Sample::pointer)
    QPointF m_lastTouchPagePoint;     // dernier point brut du doigt qui trace, en coordonnées de page
    QPointF
        m_lastRawPagePoint; // dernier point brut routé vers l'outil (avant le filtre), coordonnées de page
    QPointF m_lastTouchViewPoint; // même point, en coordonnées de vue (le doigt qui passe au geste y est)
    std::array<GestureContact, 2> m_gestureContacts;
    input::TwoFingerGesture m_gesture;
    qreal m_gestureZoom = 1.0;      // zoom au début du geste
    QPointF m_gesturePage;          // point de page sous l'ancre du geste, ramené sous le milieu des doigts
    bool m_gestureMoved = false;    // un contact du geste a bougé dans l'événement en cours
    bool m_refreshThisTurn = false; // M3 : un redessin est parti dans ce tour et n'a pas encore commencé
};

} // namespace actcraft::ui
