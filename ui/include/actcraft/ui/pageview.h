#pragma once

#include <actcraft/core/inkprobe.h>
#include <actcraft/input/gesture.h>
#include <actcraft/input/routing.h>
#include <actcraft/input/twouserpolicy.h>
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
#include <optional>
#include <vector>

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
// Appui long d'un doigt (D11-7) : durée et tolérance de mouvement, en dp (pixels logiques de la vue).
inline constexpr int kTouchHoldMs = 500;
inline constexpr qreal kTouchHoldToleranceDp = 10.0;

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
    // L4-C15 : dernier point de pointeur reçu par la vue (coordonnées de la vue), appuis et mouvements à
    // bouton enfoncé compris : le HoverHandler de QML, lui, ne bouge pas pendant un glisser.
    Q_PROPERTY(QPointF pointerPosition READ pointerPosition NOTIFY pointerPositionChanged)

public:
    using SampleFilter = std::function<input::Sample(const input::Sample&)>;

    explicit PageView(QQuickItem* parent = nullptr);
    ~PageView() override;

    render::FixedPageItem* fixedPage() const noexcept;
    void setFixedPage(render::FixedPageItem* page);
    render::InkItem* ink() const noexcept;
    void setInk(render::InkItem* ink);
    // L11-C7 (D) : encre vive du second utilisateur, juste au-dessus de ink(), aux mêmes coordonnées ; créée
    // à la première demande, remplacée avec ink() ; nulle sans ink(). Un trait vif par encre : deux traits
    // simultanés en demandent deux.
    render::InkItem* secondInk();
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
    // ; la suite du même contact ne dessine plus. Appelé avant tout changement de page et toute édition
    // annoncée (pageAboutToChange, editAboutToApply, branchés dans Main.qml) : le geste du second utilisateur
    // y est laissé à ToolHost, branché sur les mêmes annonces (L11-C13). La vue elle-même, avant tout
    // changement de zoom ou de défilement, termine aussi celui du second (endStrokes) : un trait n'est jamais
    // perdu ni versé à une autre page.
    // CONTRAT (L11-C16) : ne termine que le geste du PREMIER utilisateur (et le trait du stylo du second,
    // versé par la vue). Un geste du second passant par un outil de ToolHost n'est jamais terminé ici : il
    // l'est par la connexion de ToolHost aux mêmes annonces. Nom gardé (appelé aussi par tst_pageview).
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
    cacheRefreshCount() const noexcept; // redessins du cache terminés depuis la création (crochet S6, H10-1)
    QPointF pointerPosition() const noexcept { return m_pointerPosition; }
    bool gestureActive() const noexcept;     // un geste à deux doigts est en cours (L3-T13 ; faux au socle)
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
    void pointerPositionChanged();
    void pointerPressed(QPointF viewPos);       // appui sur la vue, qu'un outil le prenne ou non
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
    // L11-T5 : contact routé vers l'outil, de l'appui au relâché (ou jusqu'à ce que la vue le rende) :
    // décision retenue (utilisateur, outil imposé) et dernier point brut. Quelques contacts : recherche
    // linéaire, capacité réservée, aucune allocation par point.
    struct ToolContact
    {
        input::ContactKey key; // clé du routeur (L11-C1 : deux stylets ont chacun la leur)
        std::uint64_t pointer = 0;
        input::DeviceKind device = input::DeviceKind::Other;
        input::UserId user = input::kFirstUser;
        input::FingerTool fingerTool = input::FingerTool::Current;
        QPointF lastPagePoint; // dernier point brut, coordonnées de page
        QPointF lastViewPoint; // même point, coordonnées de vue (un doigt qui passe au geste y est)
    };
    bool handlePointer(const QEvent& event, QPointF viewPos);
    // Échantillon d'un événement (deviceId compris), ou rien (pas de Pipeline, appareil exclu).
    std::optional<input::Sample> sampleFor(const QEvent& event, QPointF viewPos);
    // L11-C24 : utilisateur de cet appui, où qu'il tombe dans la fenêtre ; rien si l'événement n'attribue
    // aucun appui (autre événement, souris synthétisée d'un stylet, pas de input::TwoUserPolicy).
    std::optional<input::UserId> pressUser(const QEvent& event) const;
    // Applique la décision du routeur ; pressesInEvent : appuis de doigt non couverts de l'événement.
    bool applyDecision(input::Sample& sample, const input::Decision& decision, QPointF viewPos,
                       int pressesInEvent);
    ToolContact* toolContact(const input::ContactKey& key);
    // Outil d'un contact : imposé (FingerTool), sinon l'outil courant de l'utilisateur, sinon le stylo
    // interne.
    tools::Tool* toolOf(input::UserId user, input::FingerTool fingerTool) const;
    bool anyToolBusy() const;
    void configureImposedPen(input::UserId user); // réglages « pen » de l'utilisateur, relus à l'appui
    // Relâché synthétisé au dernier point brut du contact, filtré comme les autres, puis cancel() si l'outil
    // reste occupé (trait gardé).
    void endWithRelease(const ToolContact& contact);
    void abandonContact(const ToolContact& contact); // Tool::abandon : rien n'est versé
    // Un trait peut commencer ici : vue visible, aucune fenêtre surgissante, point sur la vue.
    bool canStartStroke(QPointF viewPos) const;
    // Un élément posé sur la vue (empilé au-dessus d'elle chez son parent : barres de défilement) prendrait
    // la souris ou le doigt à ce point. Qt Quick le leur livre avant la vue ; le stylet, capté par le filtre
    // de la fenêtre, doit leur être laissé de même. Consulté à l'appui seulement, jamais par point.
    bool coveredAt(QPointF viewPos) const;
    // Seul point d'émission de pointerPressed (souris, stylet, doigt) : rien sur une couverture, dont l'appui
    // n'appartient pas à la page et ne doit pas lui reprendre le focus.
    void emitPressedUnlessCovered(QPointF viewPos);
    // Termine le trait de chaque doigt routé vers l'outil (annulation, saisie volée) par un relâché à son
    // dernier point brut, et rend son contact au routeur.
    void finishTouchStroke();
    // Corps de finishStroke ; secondUserToo : le geste du second se termine aussi, versé (zoom, défilement).
    void endStrokes(bool secondUserToo);
    // Voie Tool : filtre d'échantillons s'il est posé, puis Pipeline (numérotation, sonde, doublons), puis
    // `tool` (Pipeline::push est synchrone).
    void pushToTool(const input::Sample& sample, tools::Tool* tool);
    // Une couverture détruite est oubliée.
    void forgetDeadCovers();
    // Lot 10 : différé posé sur la page fixe quand le cache est actif et une couche d'attente branchée.
    void updateDeferral();
    // Pose le différé sur la page fixe ; le quitter verse d'abord ce qui attend (cache redessiné).
    void applyDeferral(bool deferred);
    void onPendingChanged();
    // Une entrée qui agit relance la minuterie du versement, si elle court.
    void postponeFold();
    // L3-T13 : un contact du geste à deux doigts (pointer 0 : place libre ; un doigt vaut identifiant + 1).
    struct GestureContact
    {
        std::uint64_t pointer = 0;
        QPointF viewPos;
    };
    // endToolContacts du routeur : le doigt qui écrit passe au geste (appui d'un doigt seul dans son
    // événement : trait abandonné, H3-17) ou à rien (stylet, souris, doigts posés ensemble : trait terminé là
    // où il était, M4 b, H11-O4).
    void endToolContacts(const input::Decision& decision, int pressesInEvent);
    bool onGestureSample(const input::Sample& sample, QPointF viewPos);
    bool addGestureContact(std::uint64_t pointer, QPointF viewPos); // faux : le geste a ses deux contacts
    void applyGesture();
    void endGesture(); // annulation, saisie volée : contacts du geste oubliés, au routeur aussi
    // Termine le geste à deux doigts s'il est actif, et le signale.
    void stopGesture();
    // M3 : les demandes de redessin suivantes de ce tour sont couvertes par celle qui vient de partir.
    void holdRequestsOfThisTurn();

    render::FixedPageItem* m_fixedPage = nullptr;
    render::InkItem* m_ink = nullptr;
    QPointer<render::InkItem> m_secondInk; // enfant QObject de m_ink
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
    QPointF m_pointerPosition; // voir pointerPosition
    QPointF
        m_lastRawPagePoint; // dernier point brut routé vers l'outil (avant le filtre), coordonnées de page
    input::ContactKey m_lastToolKey; // contact de ce point (finishStroke : son outil)
    std::vector<ToolContact>
        m_toolContacts; // L11-T5 : contacts routés vers l'outil (doigts : trait en cours)
    std::array<GestureContact, 2> m_gestureContacts;
    input::TwoFingerGesture m_gesture;
    qreal m_gestureZoom = 1.0;   // zoom au début du geste
    QPointF m_gesturePage;       // point de page sous l'ancre du geste, ramené sous le milieu des doigts
    bool m_gestureMoved = false; // un contact du geste a bougé dans l'événement en cours
    // M3 : un redessin est parti dans ce tour et n'a pas encore commencé ; faux à la fin du tour ou à la
    // synchronisation suivante, la première des deux (m4, L3-C8).
    bool m_refreshThisTurn = false;

    // Amendement 1 du lot 4 : ajout en fin, en-tête figé par L0-S7. V5 (L4-T0) : aucun état de contact
    // n'était publié (input::PointerRouter garde le sien privé, gestureActive ne couvre que deux doigts).
    // Vrai entre l'appui et le relâché de tout contact routé (stylet, souris, doigt ; vers l'outil ou le
    // geste), suivi dans le chemin d'entrée de la vue, sans toucher au routeur : un contact compte dès que le
    // routeur retient sa décision et jusqu'à ce qu'il la rende (relâché, ou contact rendu par la vue).
    // contactActiveChanged aux deux bascules. Les miniatures (PageThumbnails) n'en lancent aucune pendant un
    // contact.
    Q_PROPERTY(bool contactActive READ contactActive NOTIFY contactActiveChanged)

public:
    bool contactActive() const noexcept;

signals:
    void contactActiveChanged();

private:
    // L11-C1 : un contact est sa clé du routeur (input::contactKeyOf), pas Sample::pointer (0 pour tout
    // stylet).
    void holdContact(const input::ContactKey& key);    // appui retenu par le routeur (outil ou geste)
    void forgetContact(const input::ContactKey& key);  // décision rendue par le routeur (relâché)
    void releaseContact(const input::ContactKey& key); // m_router.release(key), puis forgetContact
    QList<input::ContactKey> m_heldContacts;

    // Lot 11 (L11-T5) : ajout en fin.
public:
    // Remplace la politique du routeur (nullptr : gestion complète) ; les contacts en cours gardent leur
    // décision. Posée par le contrôleur du mode du toucher (L11-T6) : la vue ne lit aucun réglage.
    void setRoutingPolicy(std::unique_ptr<input::RoutingPolicy> policy);
    // L11-C6 : premier stylet de la dernière input::TwoUserPolicy remplacée par setRoutingPolicy (0 : aucun)
    // ; DualMode le rend à celle qu'il repose après un changement du mode du toucher.
    std::uint64_t lastFirstStylusDevice() const noexcept { return m_lastFirstStylus; }
    // Utilisateur de la dernière décision d'appui routée vers l'outil ou le geste (tests).
    int lastContactUser() const noexcept;
    // L11-C24 (F-303) : utilisateur du dernier appui vu par le filtre de la fenêtre, avant sa livraison, sur
    // la vue ou hors d'elle (palette, menu). Sans sens hors du mode à deux : DualMode y veille.
    input::UserId lastPressUser() const noexcept { return m_lastPressUser; }
    // Chaque contact en cours se termine comme un relâché à sa dernière position (trait gardé) ; le geste
    // est rendu, le routeur remis à zéro, contactActive devient faux. La suite d'un contact terminé n'écrit
    // plus (le reste d'un contact de stylet reste pris par la vue jusqu'à son relâché). Appelé par la vue au
    // redimensionnement (D11-10) ; à la mise en arrière-plan par la reprise (D11-8).
    Q_INVOKABLE void finishContacts();
    bool
    touchHoldTimerActive() const noexcept; // crochet de test : armé seulement pendant un contact de doigt

signals:
    // D11-7 : un doigt routé vers l'outil est resté immobile (≤ kTouchHoldToleranceDp de son appui)
    // kTouchHoldMs ; pagePoint et windowPoint (scène) sont ceux de l'appui. Une fois par contact.
    void touchHeld(QPointF pagePoint, QPointF windowPoint, int user, const QString& tool);

private:
    std::unique_ptr<actcraft::tools::PenTool> m_imposedPen; // FingerTool::Pen sans fournisseur
    int m_lastContactUser = input::kFirstUser;
    QTimer m_holdTimer; // un seul, armé à l'appui d'un doigt routé vers l'outil, jamais par échantillon
    std::uint64_t m_holdPointer = 0;
    QPointF m_holdViewPoint;
    QPointF m_holdPagePoint;
    input::UserId m_holdUser = input::kFirstUser;
    input::FingerTool m_holdFingerTool = input::FingerTool::Current; // outil imposé du contact tenu

    // Lot 11 (L11-C1) : ajout en fin.
public:
    // Aire de l'utilisateur 1 en coordonnées de VUE (moitié de la deuxième palette), pour une politique
    // input::TwoUserPolicy posée par setRoutingPolicy : convertie en coordonnées de page (celles des
    // échantillons) à la pose de l'aire, de la politique, et à chaque changement de zoom ou de défilement.
    // Posée, elle prévaut sur l'aire donnée à la construction de la politique : toute politique à deux
    // utilisateurs posée ensuite la reçoit (par setRoutingPolicy, ou au zoom suivant si elle est posée par
    // router()). La politique est relue au routeur à chaque usage : aucun pointeur gardé.
    void setSecondUserViewArea(QRectF viewArea);

private:
    void updateSecondUserArea();
    // Stylets dont le contact, commencé sur la vue, est en cours (Sample::deviceId ; 0 : sans identifiant) :
    // chacun suivi à part, l'appui d'un autre ne le coupe pas. Réservé à la construction : aucune allocation
    // pour quelques stylets. std::vector et non QVarLengthArray : sa croissance (qalloc.h) déclenche C4702
    // sous MSVC /W4 /WX.
    bool stylusDrawing(std::uint64_t device) const;
    void setStylusDrawing(std::uint64_t device, bool drawing);
    std::vector<std::uint64_t> m_stylusesDrawing;
    std::optional<QRectF> m_secondUserViewArea;
    std::uint64_t m_lastFirstStylus = 0;               // L11-C6
    input::UserId m_lastPressUser = input::kFirstUser; // L11-C24
    // Horodatage du dernier événement de toucher à appuis : Qt Quick livre deux fois [A relâché, B appuyé]
    // à la vue ; la seconde livraison n'a aucun effet (L11-C1).
    quint64 m_lastPressEventTime = 0;
    std::vector<int> m_lastPressedIds; // points appuyés de cet événement (réservé)
    bool m_lastPressTaken = false;     // ce que la première livraison a pris
    bool isSecondDelivery(const QTouchEvent& event) const;
};

} // namespace actcraft::ui
