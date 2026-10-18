#pragma once

#include <actcraft/core/model.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/objectrenderer.h>
#include <actcraft/render/paintedrenderer.h>

#include <QPointer>
#include <QQuickItem>

#include <atomic>
#include <cstdint>
#include <utility>
#include <vector>

namespace actcraft::render {

// Lot 10 (DA11) : dessine, au-dessus du cache, les objets de la page fixe qui attendent leur versement
// (FixedPageItem::pendingIds), avec les nœuds mêmes de la page fixe (createObjectNode). Sœur du cache dans la
// scène (jamais enfant de la page fixe : hideSource du cache la cacherait). Aucune image n'attend
// (FixedPageItem::insert les fait reconstruire) : pas de magasin d'images ici.
class PendingLayerItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(
        actcraft::render::FixedPageItem* fixedPage READ fixedPage WRITE setFixedPage NOTIFY fixedPageChanged)

public:
    explicit PendingLayerItem(QQuickItem* parent = nullptr);
    FixedPageItem* fixedPage() const noexcept;
    void setFixedPage(FixedPageItem* page);
    int builtCount() const noexcept; // crochet de test : nœuds posés à la dernière synchronisation

signals:
    void fixedPageChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData* data) override;

private:
    QPointer<FixedPageItem> m_page;
    QMetaObject::Connection m_pendingConnection;
    QMetaObject::Connection m_contentConnection;
    QMetaObject::Connection m_readyConnection;
    RendererRegistry m_registry;
    PaintedRenderer m_painted;
    std::vector<std::pair<core::ObjectId, QSGNode*>> m_nodes; // fil de rendu ; nœud nul : rien à dessiner
    std::uint64_t m_builtRevision = 0;
    std::atomic<int> m_builtCount{0};
};

} // namespace actcraft::render
