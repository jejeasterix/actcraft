#pragma once

#include <actcraft/core/inkprobe.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
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

// Épaisseur du stylo, pixels de page (F-137 : réglette 1 à 100 ; F-220 : 4 par défaut).
inline constexpr qreal kDefaultPenWidth = 4.0;
inline constexpr qreal kMinPenWidth = 1.0;
inline constexpr qreal kMaxPenWidth = 100.0;

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
    Q_PROPERTY(QColor penColor READ penColor WRITE setPenColor NOTIFY penChanged)
    Q_PROPERTY(qreal penWidth READ penWidth WRITE setPenWidth NOTIFY penChanged)
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
    // Banc : sonde de latence (DA4) et numérotation des échantillons depuis `next`.
    void setInkProbe(core::InkProbe* probe);
    void resetInputSequence(std::uint64_t next = 0);
    // Appelé par l'adaptateur du document quand un trait est figé.
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
    // Vrai quand la texture du cache correspond à la page et au zoom courants (toujours vrai sans cache).
    bool cacheReady() const noexcept;
    const ViewTransform& viewTransform() const noexcept;
    QColor penColor() const noexcept;
    // Couleur invalide ignorée.
    void setPenColor(const QColor& color);
    qreal penWidth() const noexcept;
    // Bornée à [kMinPenWidth, kMaxPenWidth].
    void setPenWidth(qreal width);
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

    render::FixedPageItem* m_fixedPage = nullptr;
    render::InkItem* m_ink = nullptr;
    core::InkProbe* m_probe = nullptr;
    std::unique_ptr<InkSurfaceAdapter> m_inkSurface;
    std::unique_ptr<FixedPageSink> m_documentSink;
    std::unique_ptr<tools::PenTool> m_pen;
    std::unique_ptr<input::Pipeline> m_pipeline;
    QMetaObject::Connection m_readyConnection;
    QPointer<QQuickWindow> m_filteredWindow; // variante B : le stylet est capté par un filtre sur la fenêtre

    QQuickItem* m_content = nullptr;
    ViewTransform m_transform;
    bool m_fitMode = true;
    bool m_cacheEnabled = true;
    bool m_cacheReady = false;
    bool m_refreshInFlight = false; // un redessin du cache est demandé et pas encore terminé
    bool m_refreshAgain = false;    // une demande est arrivée pendant ce redessin : en refaire un à sa fin
    QSize m_cacheTextureSize;
    QTimer m_cacheTimer;
    QList<const QInputDevice*> m_exclusive;
    QColor m_penColor = Qt::black;
    qreal m_penWidth = kDefaultPenWidth;
    bool m_inputBlocked = false;
    bool m_stylusDrawing = false; // un trait du stylet, commencé sur la vue, est en cours
    bool m_touchDrawing = false;  // un trait d'un doigt est en cours
    QPointF m_lastTouchViewPos;
    QPointF m_lastPagePoint; // dernier point transmis au stylo, en coordonnées de page
};

} // namespace actcraft::ui
