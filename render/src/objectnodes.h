#pragma once

#include <actcraft/core/model.h>
#include <actcraft/render/objectrenderer.h>
#include <actcraft/render/paintedrenderer.h>

#include <cstddef>

class QSGNode;

namespace actcraft::render {

// Nœud d'un objet de page, tel que la page fixe le pose (et la couche d'attente, lot 10) : moteur du
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

ObjectNode createObjectNode(const core::PageObject& object, std::size_t index,
                            const RendererRegistry& registry, const PaintedRenderer& painter,
                            const RenderContext& context, const RenderContext& painted);

} // namespace actcraft::render
