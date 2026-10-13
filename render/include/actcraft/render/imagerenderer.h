#pragma once

#include <actcraft/render/objectrenderer.h>

namespace actcraft::render {

// QSGImageNode : Qt le fournit pour le RHI et pour le rendu logiciel ; un seul chemin suffit.
class ImageRenderer final : public ObjectRenderer
{
public:
    core::ObjectKind kind() const noexcept override { return core::ObjectKind::Image; }
    QSGNode* createNode(const core::PageObject& object, std::size_t objectIndex,
                        const RenderContext& context) const override;
};

} // namespace actcraft::render
