#pragma once

#include <actcraft/bench/memory.h>
#include <actcraft/bench/stats.h>
#include <actcraft/core/inkprobe.h>

#include <QObject>
#include <QString>

#include <memory>
#include <mutex>
#include <utility>
#include <vector>

class QQuickWindow;

namespace actcraft::bench {

// Collecte sur le fil de rendu (frameSwapped, mémoire) et sert de sonde d'encre (DA4). Les connexions à la
// fenêtre sont directes (fil de rendu) : le collecteur doit survivre à la fenêtre.
class FrameRecorder : public QObject, public core::InkProbe
{
    Q_OBJECT

public:
    explicit FrameRecorder(QQuickWindow* window, QObject* parent = nullptr);

    void begin(); // vide les séries et active l'enregistrement
    void end();

    void sampleReceived(std::uint64_t seq, std::int64_t nowNs) override;
    void samplesSynced(std::uint64_t lastSeq, std::int64_t nowNs) override;

    std::vector<std::int64_t> swapNs() const;
    // Début de chaque synchronisation (beforeSynchronizing) : le fil principal y est arrêté.
    std::vector<std::int64_t> syncStartNs() const;
    // Chaque synchronisation est rattachée au frameSwapped qui la suit (l'image qu'elle prépare).
    std::vector<SyncRecord> syncRecords() const;
    std::vector<std::pair<std::uint64_t, std::int64_t>> received() const;
    MemorySummary memory() const;
    QString renderLoop() const; // basic | threaded | unknown
    QString gpuName() const;

private:
    void onBeforeSynchronizing(); // fil de rendu, fil principal arrêté
    void onBeforeRendering();     // fil de rendu
    void onFrameSwapped();        // fil de rendu

    QQuickWindow* m_window;
    mutable std::mutex m_mutex;
    bool m_recording = false;
    std::vector<std::int64_t> m_swaps;
    std::vector<std::int64_t> m_syncStarts;
    std::vector<std::pair<std::uint64_t, std::int64_t>> m_pendingSyncs;
    std::vector<SyncRecord> m_syncs;
    std::vector<std::pair<std::uint64_t, std::int64_t>> m_received;
    MemoryTracker m_memory;
    std::unique_ptr<GpuMemoryReader> m_gpu; // créé et lu sur le fil de rendu seulement
    bool m_gpuTried = false;
    QString m_gpuName;
    QString m_renderLoop = QStringLiteral("unknown");
};

} // namespace actcraft::bench
