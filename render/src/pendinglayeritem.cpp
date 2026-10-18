#include "objectnodes.h"

#include <actcraft/render/pendinglayeritem.h>

#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSGTransformNode>

namespace actcraft::render {

namespace {
// Retire et détruit tous les nœuds posés par la couche.
void clearNodes(QSGNode* root, std::vector<std::pair<core::ObjectId, QSGNode*>>& nodes)
{
    while (QSGNode* child = root->firstChild()) {
        root->removeChildNode(child);
        delete child;
    }
    nodes.clear();
}
} // namespace

PendingLayerItem::PendingLayerItem(QQuickItem* parent)
    : QQuickItem(parent), m_registry(RendererRegistry::withDefaults())
{
    setFlag(ItemHasContents);
}

FixedPageItem* PendingLayerItem::fixedPage() const noexcept
{
    return m_page.data();
}

void PendingLayerItem::setFixedPage(FixedPageItem* page)
{
    if (page == m_page) {
        return;
    }
    disconnect(m_pendingConnection);
    disconnect(m_contentConnection);
    disconnect(m_readyConnection);
    m_page = page;
    m_pageSwitched = true;
    if (m_page) {
        m_pendingConnection = connect(m_page, &FixedPageItem::pendingChanged, this, [this] { update(); });
        m_contentConnection = connect(m_page, &FixedPageItem::contentChanged, this, [this] { update(); });
        // Au prêt, la page fixe se reconstruit (avec ce qui attendait) : la couche se vide dans la même
        // image.
        m_readyConnection = connect(m_page, &FixedPageItem::readyChanged, this, [this] { update(); });
    }
    update();
    emit fixedPageChanged();
}

int PendingLayerItem::builtCount() const noexcept
{
    return m_builtCount.load();
}

// Fil de rendu, fil principal bloqué : la page fixe et ses objets en attente sont lus tels que le fil
// principal les a laissés, comme la scène de la page fixe le fait dans la même synchronisation.
QSGNode* PendingLayerItem::updatePaintNode(QSGNode* old, UpdatePaintNodeData*)
{
    const bool software = window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software;
    // Logiciel : racine de transformation (identité), comme la page fixe (piège du moteur logiciel 6.12).
    QSGNode* root = old ? old : (software ? new QSGTransformNode : new QSGNode);
    if (!old) {
        // Graphe recréé (couche remise dans une fenêtre, scène invalidée) : les anciens nœuds ont été
        // détruits avec lui ; tout ce qui attend est à reposer.
        m_nodes.clear();
    }
    if (m_pageSwitched) {
        // Autre page fixe : ses objets en attente n'ont rien de commun avec les nœuds posés, même à
        // identifiants égaux (chaque document compte les siens).
        clearNodes(root, m_nodes);
        m_pageSwitched = false;
    }
    // Tant que la page fixe redécode (reconstruction avec images, autre page), elle garde ses anciens nœuds,
    // sans les objets qui attendaient : la couche garde les siens jusqu'au prêt (readyChanged), sinon ils
    // disparaîtraient de l'écran.
    if (m_page && !m_page->isReady()) {
        return root;
    }
    static const std::vector<core::ObjectId> none;
    const std::vector<core::ObjectId>& wanted = m_page ? m_page->pendingIds() : none;
    const std::uint64_t revision = m_page ? m_page->pendingRevision() : 0;
    if (m_page && wanted.size() > m_page->page().objects.size()) {
        // Invariant rompu (FixedPageItem le tient : ne doit pas arriver) : ne rien dessiner plutôt que lire
        // hors de la page.
        clearNodes(root, m_nodes);
        m_builtCount.store(0);
        return root;
    }
    // Les nœuds déjà posés restent s'ils sont le début de ce qui attend (traits ajoutés un à un) ; sinon
    // (versement, retrait, modification en place) la couche refait tout.
    bool keep = revision == m_builtRevision && m_nodes.size() <= wanted.size();
    for (std::size_t i = 0; keep && i < m_nodes.size(); ++i) {
        keep = m_nodes[i].first == wanted[i];
    }
    if (!keep) {
        clearNodes(root, m_nodes);
        m_builtRevision = revision;
    }
    if (m_page && m_nodes.size() < wanted.size()) {
        const core::Page& shown = m_page->page();
        RenderContext context;
        context.window = window();
        context.software = software;
        context.images = nullptr;
        context.rasterScale = m_page->rasterScale();
        RenderContext painted = context;
        painted.software = software && !m_page->forceRasterized();
        // Les objets en attente sont les derniers de la page (invariant tenu par FixedPageItem).
        const std::size_t first = shown.objects.size() - wanted.size();
        for (std::size_t k = m_nodes.size(); k < wanted.size(); ++k) {
            const std::size_t index = first + k;
            const ObjectNode made =
                createObjectNode(shown.objects[index], index, m_registry, m_painted, context, painted);
            m_nodes.emplace_back(wanted[k], made.root);
            if (made.root) {
                root->appendChildNode(made.root);
            }
        }
    }
    int built = 0;
    for (const auto& entry : m_nodes) {
        built += entry.second ? 1 : 0;
    }
    m_builtCount.store(built);
    return root;
}

} // namespace actcraft::render
