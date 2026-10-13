#pragma once

#include <actcraft/render/objectrenderer.h>

namespace actcraft::render {

class ShapeRenderer final : public ObjectRenderer
{
public:
    core::ObjectKind kind() const noexcept override { return core::ObjectKind::Shape; }
    QSGNode* createNode(const core::PageObject& object, std::size_t objectIndex,
                        const RenderContext& context) const override;
};

} // namespace actcraft::render
