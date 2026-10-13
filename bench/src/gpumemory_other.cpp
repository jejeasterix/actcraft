#include <actcraft/bench/memory.h>

namespace actcraft::bench {

std::unique_ptr<GpuMemoryReader> GpuMemoryReader::create(QQuickWindow*)
{
    return nullptr;
}

} // namespace actcraft::bench
