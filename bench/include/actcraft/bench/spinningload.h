#pragma once

#include <atomic>
#include <cstdint>
#include <thread>

namespace actcraft::bench {

// L5-C14 : charge témoin de « export-fond-reference » (voir ScenarioPlan::spinsInBackground) : un fil qui
// calcule sans fin (aucune écriture, aucun partage avec l'interface), arrêté et joint à la destruction. Il
// garde un cœur actif comme l'export, sans en avoir le coût. `iterations()` compte ses tours : la preuve
// que le fil a tourné, sans chrono.
class SpinningLoad
{
public:
    SpinningLoad()
        : m_thread([this] {
              std::uint64_t x = 88172645463325252ULL;
              while (!m_stop.load(std::memory_order_relaxed)) {
                  for (int i = 0; i < 100'000; ++i) {
                      x ^= x << 13;
                      x ^= x >> 7;
                      x ^= x << 17;
                  }
                  m_sink.store(x, std::memory_order_relaxed); // le calcul n'est pas supprimé
                  m_iterations.fetch_add(1, std::memory_order_relaxed);
              }
          })
    {}
    ~SpinningLoad()
    {
        m_stop.store(true, std::memory_order_relaxed);
        m_thread.join();
    }
    SpinningLoad(const SpinningLoad&) = delete;
    SpinningLoad& operator=(const SpinningLoad&) = delete;

    std::int64_t iterations() const noexcept { return m_iterations.load(std::memory_order_relaxed); }

private:
    std::atomic<bool> m_stop{false};
    std::atomic<std::uint64_t> m_sink{0};
    std::atomic<std::int64_t> m_iterations{0};
    std::thread m_thread; // déclaré en dernier : les atomiques existent avant son départ
};

} // namespace actcraft::bench
