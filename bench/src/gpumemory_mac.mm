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
    // Ce fichier est compilé sans ARC : on retient le périphérique, et le destructeur le relâche.
    explicit MetalGpuMemory(id<MTLDevice> device) : m_device([device retain]) {}
    ~MetalGpuMemory() override { [m_device release]; }
    MetalGpuMemory(const MetalGpuMemory&) = delete;
    MetalGpuMemory& operator=(const MetalGpuMemory&) = delete;

    std::optional<std::uint64_t> currentBytes() override
    {
        return static_cast<std::uint64_t>([m_device currentAllocatedSize]);
    }

    QString adapterName() const override { return QString::fromNSString([m_device name]); }

private:
    id<MTLDevice> m_device; // retenu (+1) : le Runner est détruit après la fenêtre, qui ne garantit pas sa durée
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
