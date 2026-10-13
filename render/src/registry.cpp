#include <actcraft/render/imagerenderer.h>
#include <actcraft/render/objectrenderer.h>
#include <actcraft/render/shaperenderer.h>
#include <actcraft/render/strokerenderer.h>

namespace actcraft::render {

void RendererRegistry::add(std::unique_ptr<ObjectRenderer> renderer)
{
    const auto index = static_cast<std::size_t>(renderer->kind());
    m_renderers[index] = std::move(renderer);
}

const ObjectRenderer* RendererRegistry::find(core::ObjectKind kind) const noexcept
{
    return m_renderers[static_cast<std::size_t>(kind)].get();
}

std::size_t RendererRegistry::size() const noexcept
{
    std::size_t count = 0;
    for (const auto& r : m_renderers) {
        count += r ? 1u : 0u;
    }
    return count;
}

RendererRegistry RendererRegistry::withDefaults()
{
    RendererRegistry registry;
    registry.add(std::make_unique<ShapeRenderer>());
    registry.add(std::make_unique<ImageRenderer>());
    registry.add(std::make_unique<StrokeRenderer>());
    return registry;
}

} // namespace actcraft::render
