#include <actcraft/bench/memory.h>

#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QString>

#import <Metal/Metal.h>

namespace actcraft::bench {

namespace {

class MetalGpuMemory final : public GpuMemoryReader
{
public:
    explicit MetalGpuMemory(id<MTLDevice> device) : m_device(device) {}

    std::optional<std::uint64_t> currentBytes() override
    {
        return static_cast<std::uint64_t>([m_device currentAllocatedSize]);
    }

    QString adapterName() const override { return QString::fromNSString([m_device name]); }

private:
    id<MTLDevice> m_device; // appartient à la fenêtre, qui vit plus longtemps que le banc
};

} // namespace

std::unique_ptr<GpuMemoryReader> GpuMemoryReader::create(QQuickWindow* window)
{
    if (!window || window->rendererInterface()->graphicsApi() != QSGRendererInterface::Metal) {
        return nullptr;
    }
    void* resource = window->rendererInterface()->getResource(window, QSGRendererInterface::DeviceResource);
    if (!resource) {
        return nullptr;
    }
    return std::make_unique<MetalGpuMemory>((__bridge id<MTLDevice>)resource);
}

} // namespace actcraft::bench
