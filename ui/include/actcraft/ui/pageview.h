#pragma once

#include <actcraft/core/inkprobe.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/ui/viewtransform.h>

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

protected:
    void wheelEvent(QWheelEvent* event) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    bool acceptsDevice(const QInputDevice* device) const;
    bool event(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void itemChange(ItemChange change, const ItemChangeData& value) override;

private:
    QSizeF pageSize() const;
    void apply(const ViewTransform& transform);
    void updateCacheTextureSize();
    void setCacheReady(bool ready);
    void rebuildPen();
    bool handlePointer(const QEvent& event, QPointF viewPos);

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
};

} // namespace actcraft::ui
