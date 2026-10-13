#pragma once

#include <actcraft/render/objectrenderer.h>

namespace actcraft::render {

class StrokeRenderer final : public ObjectRenderer
{
public:
    core::ObjectKind kind() const noexcept override { return core::ObjectKind::Stroke; }
    QSGNode* createNode(const core::PageObject& object, std::size_t objectIndex,
                        const RenderContext& context) const override;
};

} // namespace actcraft::render
