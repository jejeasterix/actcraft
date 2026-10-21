#pragma once

#include <actcraft/core/model.h>
#include <actcraft/render/objectrenderer.h>
#include <actcraft/render/paintedrenderer.h>

#include <cstddef>

class QQuickWindow;
class QSGNode;

namespace actcraft::render {

// Nœud d'un objet de page, tel que la page fixe le pose (et les couches d'attente et soulevée) : moteur du
// prototype s'il sait faire, sinon moteur peint ; image illisible : repère du peintre, comme la miniature ;
// transformation et translucidité par nœuds parents. root == nullptr : objet invisible, groupe, ou rien à
// dessiner. leaf : nœud du moteur (avant enveloppe) ; viaPainter : il vient du moteur peint (en RHI, nœud
// rastérisé à refaire quand l'échelle change).
struct ObjectNode
{
    QSGNode* root = nullptr;
    QSGNode* leaf = nullptr;
    bool viaPainter = false;
};

// Contextes de createObjectNode, communs à la page fixe, à la couche d'attente et à la couche soulevée :
// `context` pour le moteur du prototype ; `painted` pour le moteur peint, le fond peint et la grille,
// vectoriels en rendu logiciel, textures en RHI ou quand la rastérisation est forcée (décision 37). La
// couche d'attente passe images == nullptr : aucune image n'attend (FixedPageItem::insert les fait
// reconstruire).
struct NodeContexts
{
    RenderContext context;
    RenderContext painted;
};

ObjectNode createObjectNode(const core::PageObject& object, std::size_t index,
                            const RendererRegistry& registry, const PaintedRenderer& painter,
                            const NodeContexts& contexts);

// Retire et détruit tous les enfants de `root`.
void clearChildren(QSGNode* root);

NodeContexts nodeContexts(QQuickWindow* window, bool software, const ImageStore* images, double rasterScale,
                          bool forceRasterized);

} // namespace actcraft::render
