#include <actcraft/bench/recorder.h>
#include <actcraft/core/clock.h>

#include <QCoreApplication>
#include <QQuickWindow>
#include <QThread>

namespace actcraft::bench {

FrameRecorder::FrameRecorder(QQuickWindow* window, QObject* parent) : QObject(parent), m_window(window)
{
    connect(window, &QQuickWindow::beforeSynchronizing, this, &FrameRecorder::onBeforeSynchronizing,
            Qt::DirectConnection);
    connect(window, &QQuickWindow::beforeRendering, this, &FrameRecorder::onBeforeRendering,
            Qt::DirectConnection);
    connect(window, &QQuickWindow::frameSwapped, this, &FrameRecorder::onFrameSwapped, Qt::DirectConnection);
}

void FrameRecorder::begin()
{
    std::lock_guard lock(m_mutex);
    m_swaps.clear();
    m_syncStarts.clear();
    m_pendingSyncs.clear();
    m_syncs.clear();
    m_received.clear();
    m_memory.reset();
    m_recording = true;
}

void FrameRecorder::end()
{
    std::lock_guard lock(m_mutex);
    m_recording = false;
}

void FrameRecorder::sampleReceived(std::uint64_t seq, std::int64_t nowNs)
{
    std::lock_guard lock(m_mutex);
    if (m_recording) {
        m_received.emplace_back(seq, nowNs);
    }
}

void FrameRecorder::samplesSynced(std::uint64_t lastSeq, std::int64_t nowNs)
{
    std::lock_guard lock(m_mutex);
    if (m_recording) {
        m_pendingSyncs.emplace_back(lastSeq, nowNs);
    }
}

void FrameRecorder::onBeforeSynchronizing()
{
    const std::int64_t now = core::monotonicNowNs();
    std::lock_guard lock(m_mutex);
    if (m_recording) {
        m_syncStarts.push_back(now);
    }
}

void FrameRecorder::onBeforeRendering()
{
    if (m_gpuTried) {
        return;
    }
    m_gpuTried = true;
    auto gpu = GpuMemoryReader::create(m_window);
    std::lock_guard lock(m_mutex);
    if (gpu) {
        m_gpuName = gpu->adapterName();
    }
    m_gpu = std::move(gpu);
}

void FrameRecorder::onFrameSwapped()
{
    const std::int64_t now = core::monotonicNowNs();
    const bool onMainThread = QThread::currentThread() == QCoreApplication::instance()->thread();
    bool recording = false;
    {
        std::lock_guard lock(m_mutex);
        m_renderLoop = onMainThread ? QStringLiteral("basic") : QStringLiteral("threaded");
        recording = m_recording;
        if (recording) {
            m_swaps.push_back(now);
            // Les synchronisations en attente appartiennent à l'image qui vient d'être présentée.
            for (const auto& [lastSeq, syncNs] : m_pendingSyncs) {
                m_syncs.push_back({lastSeq, syncNs, now});
            }
            m_pendingSyncs.clear();
        }
    }
    if (recording) {
        // Valeurs instantanées : relevées à chaque image pour saisir le pic (décision 26).
        m_memory.sample(readProcessMemory(), m_gpu ? m_gpu->currentBytes() : std::nullopt);
    }
}

std::vector<std::int64_t> FrameRecorder::swapNs() const
{
    std::lock_guard lock(m_mutex);
    return m_swaps;
}

std::vector<std::int64_t> FrameRecorder::syncStartNs() const
{
    std::lock_guard lock(m_mutex);
    return m_syncStarts;
}

std::vector<SyncRecord> FrameRecorder::syncRecords() const
{
    std::lock_guard lock(m_mutex);
    return m_syncs;
}

std::vector<std::pair<std::uint64_t, std::int64_t>> FrameRecorder::received() const
{
    std::lock_guard lock(m_mutex);
    return m_received;
}

MemorySummary FrameRecorder::memory() const
{
    return m_memory.summary();
}

QString FrameRecorder::renderLoop() const
{
    std::lock_guard lock(m_mutex);
    return m_renderLoop;
}

QString FrameRecorder::gpuName() const
{
    std::lock_guard lock(m_mutex);
    return m_gpuName;
}

} // namespace actcraft::bench
