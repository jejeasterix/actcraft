#include <actcraft/bench/memory.h>

#include <QtGlobal>

#include <algorithm>

#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on
#elif defined(Q_OS_MACOS)
#include <mach/mach.h>
#endif

namespace actcraft::bench {

ProcessMemory readProcessMemory() noexcept
{
    ProcessMemory m;
#if defined(Q_OS_WIN)
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
                             sizeof(counters))) {
        m.privateBytes = counters.PrivateUsage;
        m.workingSetBytes = counters.WorkingSetSize;
        m.peakWorkingSetBytes = counters.PeakWorkingSetSize;
    }
#elif defined(Q_OS_MACOS)
    task_vm_info_data_t info{};
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_VM_INFO, reinterpret_cast<task_info_t>(&info), &count) ==
        KERN_SUCCESS) {
        m.privateBytes = static_cast<std::uint64_t>(info.phys_footprint);
        m.workingSetBytes = static_cast<std::uint64_t>(info.resident_size);
        m.peakWorkingSetBytes = static_cast<std::uint64_t>(info.resident_size_peak);
    }
#endif
    return m;
}

void MemoryTracker::reset()
{
    std::lock_guard lock(m_mutex);
    m_started = false;
    m_summary = MemorySummary{};
}

void MemoryTracker::sample(const ProcessMemory& process, std::optional<std::uint64_t> gpu)
{
    std::lock_guard lock(m_mutex);
    if (!m_started) {
        m_started = true;
        m_summary.privateStart = process.privateBytes;
        m_summary.gpuStart = gpu;
    }
    m_summary.privateEnd = process.privateBytes;
    m_summary.privatePeak = std::max(m_summary.privatePeak, process.privateBytes);
    m_summary.workingSetPeak =
        std::max({m_summary.workingSetPeak, process.workingSetBytes, process.peakWorkingSetBytes});
    m_summary.gpuEnd = gpu;
    if (gpu) {
        m_summary.gpuPeak = std::max(m_summary.gpuPeak.value_or(0), *gpu);
    }
}

MemorySummary MemoryTracker::summary() const
{
    std::lock_guard lock(m_mutex);
    return m_summary;
}

} // namespace actcraft::bench
