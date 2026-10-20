#include "objectnodes.h"

#include <actcraft/render/liftlayeritem.h>

#include <QMatrix4x4>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSGTransformNode>
#include <QTransform>

#include <utility>
#include <variant>

namespace actcraft::render {

namespace {
QMatrix4x4 matrixOf(const core::Transform& t)
{
    return QMatrix4x4(QTransform(t.m11, t.m12, t.m21, t.m22, t.dx, t.dy));
}

// Objet dont les images se cherchent dans le magasin de la page fixe (ressource, vignette de média) : tout
// objet sauf l'image synthétique (choix de L3-C1, quand le magasin la rangeait par indice de page). Depuis
// L3-C13, il la range par synthesisKey ; la lui passer est hors du périmètre de L3-C13. Sans magasin, le
// peintre la synthétise lui-même.
bool imagesByKey(const core::PageObject& object)
{
    const auto* image = std::get_if<core::ImageObject>(&object);
    return image == nullptr || !image->resource.empty();
}
} // namespace

LiftLayerItem::LiftLayerItem(QQuickItem* parent)
    : QQuickItem(parent), m_registry(RendererRegistry::withDefaults())
{
    setFlag(ItemHasContents);
}

FixedPageItem* LiftLayerItem::fixedPage() const noexcept
{
    return m_fixedPage.data();
}

void LiftLayerItem::setFixedPage(FixedPageItem* page)
{
    if (page == m_fixedPage) {
        return;
    }
    m_fixedPage = page;
    m_rebuild = true; // autre contexte (échelle de rastérisation, chemin forcé) : les nœuds sont à refaire
    update();
    emit fixedPageChanged();
}

void LiftLayerItem::setObjects(std::vector<core::PageObject> objects)
{
    m_objects = std::move(objects);
    m_extra = core::Transform{};
    m_rebuild = true;
    update();
    emit objectsChanged();
}

void LiftLayerItem::setExtraTransform(const core::Transform& extra)
{
    // Matrice seule : la synchronisation la pose sur la racine ; aucun nœud d'objet n'est refait.
    m_extra = extra;
    update();
}

void LiftLayerItem::clear()
{
    m_objects.clear();
    m_extra = core::Transform{};
    m_rebuild = true;
    update();
    emit objectsChanged();
}

int LiftLayerItem::objectCount() const noexcept
{
    return static_cast<int>(m_objects.size());
}

int LiftLayerItem::builtCount() const noexcept
{
    return m_builtCount.load();
}

// Fil de rendu, fil principal bloqué : les copies, la matrice et le contexte de la page fixe sont lus tels
// que le fil principal les a laissés.
QSGNode* LiftLayerItem::updatePaintNode(QSGNode* old, UpdatePaintNodeData*)
{
    // Racine de transformation dans les deux chemins : sa matrice est la transformation supplémentaire (en
    // logiciel, c'est aussi la racine qu'exige le piège des nœuds ajoutés sous une racine déjà rendue).
    auto* root = old ? static_cast<QSGTransformNode*>(old) : new QSGTransformNode;
    if (m_rebuild || !old) {
        // Graphe recréé (old nul) : les anciens nœuds sont partis avec lui ; sinon on les retire ici.
        while (QSGNode* child = root->firstChild()) {
            root->removeChildNode(child);
            delete child;
        }
        if (!m_objects.empty()) {
            const bool software =
                window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software;
            // Contexte de la page fixe (échelle de rastérisation, chemin forcé), comme la couche d'attente :
            // mêmes moteurs, même rendu. Magasin de la page fixe (L3-C1) : il range les ressources par clé et
            // ne les oublie qu'à un nouveau décodage, que le soulèvement d'une image à ressource ne lance pas
            // (ImageStore::covers) ; l'image soulevée y est déjà décodée. Ni second décodage, ni copie du
            // magasin. Image synthétique : sans magasin (imagesByKey).
            const FixedPageItem* page = m_fixedPage.data();
            const double scale = page ? page->rasterScale() : 1.0;
            const bool forced = page != nullptr && page->forceRasterized();
            const ImageStore* store = page ? &page->images() : nullptr;
            const NodeContexts byKey = nodeContexts(window(), software, store, scale, forced);
            const NodeContexts unkeyed = nodeContexts(window(), software, nullptr, scale, forced);
            for (std::size_t i = 0; i < m_objects.size(); ++i) {
                const NodeContexts& contexts = imagesByKey(m_objects[i]) ? byKey : unkeyed;
                const ObjectNode made = createObjectNode(m_objects[i], i, m_registry, m_painted,
                                                         contexts.context, contexts.painted);
                if (made.root) {
                    root->appendChildNode(made.root); // ordre du vecteur, comme la page fixe
                    ++m_builtCount;
                }
            }
        }
        m_rebuild = false;
    }
    const QMatrix4x4 matrix = matrixOf(m_extra);
    if (root->matrix() != matrix) {
        root->setMatrix(matrix);
    }
    return root;
}

} // namespace actcraft::render
