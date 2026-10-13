#pragma once

#include <QString>

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>

class QQuickWindow;

namespace actcraft::bench {

struct ProcessMemory
{
    std::uint64_t privateBytes = 0;        // Windows : PrivateUsage ; macOS : phys_footprint
    std::uint64_t workingSetBytes = 0;     // ensemble de travail / résident
    std::uint64_t peakWorkingSetBytes = 0; // pic tenu par le système
};

// Valeurs instantanées ; zéros sur une plateforme non prise en charge.
ProcessMemory readProcessMemory() noexcept;

// Mémoire graphique déclarée par le pilote (décision 26).
class GpuMemoryReader
{
public:
    virtual ~GpuMemoryReader() = default;
    // Fil de rendu. nullptr si le rendu n'expose pas sa mémoire (logiciel, plateforme non prise en charge).
    static std::unique_ptr<GpuMemoryReader> create(QQuickWindow* window);
    virtual std::optional<std::uint64_t> currentBytes() = 0;
    virtual QString adapterName() const = 0;
};

struct MemorySummary
{
    std::uint64_t privateStart = 0;
    std::uint64_t privateEnd = 0;
    std::uint64_t privatePeak = 0;
    std::uint64_t workingSetPeak = 0;
    std::optional<std::uint64_t> gpuStart;
    std::optional<std::uint64_t> gpuEnd;
    std::optional<std::uint64_t> gpuPeak;
};

// Début, fin et pic d'un scénario ; sample() est appelé à chaque image depuis le fil de rendu.
class MemoryTracker
{
public:
    void reset();
    void sample(const ProcessMemory& process, std::optional<std::uint64_t> gpu);
    MemorySummary summary() const;

private:
    mutable std::mutex m_mutex;
    bool m_started = false;
    MemorySummary m_summary;
};

} // namespace actcraft::bench
