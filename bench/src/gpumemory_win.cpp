#include <actcraft/bench/memory.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <QQuickWindow>
#include <QSGRendererInterface>

#include <d3d11.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

namespace actcraft::bench {

namespace {

using Microsoft::WRL::ComPtr;

class D3D11GpuMemory final : public GpuMemoryReader
{
public:
    D3D11GpuMemory(ComPtr<IDXGIAdapter3> adapter, QString name)
        : m_adapter(std::move(adapter)), m_name(std::move(name))
    {}

    std::optional<std::uint64_t> currentBytes() override
    {
        DXGI_QUERY_VIDEO_MEMORY_INFO local{};
        DXGI_QUERY_VIDEO_MEMORY_INFO nonLocal{};
        if (FAILED(m_adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &local))) {
            return std::nullopt;
        }
        if (FAILED(m_adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &nonLocal))) {
            nonLocal.CurrentUsage = 0;
        }
        return static_cast<std::uint64_t>(local.CurrentUsage + nonLocal.CurrentUsage);
    }

    QString adapterName() const override { return m_name; }

private:
    ComPtr<IDXGIAdapter3> m_adapter;
    QString m_name;
};

} // namespace

std::unique_ptr<GpuMemoryReader> GpuMemoryReader::create(QQuickWindow* window)
{
    if (!window || window->rendererInterface()->graphicsApi() != QSGRendererInterface::Direct3D11) {
        return nullptr;
    }
    auto* device = static_cast<ID3D11Device*>(
        window->rendererInterface()->getResource(window, QSGRendererInterface::DeviceResource));
    if (!device) {
        return nullptr;
    }
    ComPtr<IDXGIDevice> dxgiDevice;
    if (FAILED(device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)))) {
        return nullptr;
    }
    ComPtr<IDXGIAdapter> adapter;
    if (FAILED(dxgiDevice->GetAdapter(&adapter))) {
        return nullptr;
    }
    ComPtr<IDXGIAdapter3> adapter3;
    if (FAILED(adapter.As(&adapter3))) {
        return nullptr;
    }
    DXGI_ADAPTER_DESC desc{};
    adapter3->GetDesc(&desc);
    return std::make_unique<D3D11GpuMemory>(adapter3, QString::fromWCharArray(desc.Description));
}

} // namespace actcraft::bench
