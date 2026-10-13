#pragma once

#include <chrono>
#include <cstdint>

namespace actcraft::core {

// Horloge monotone unique du prototype (entrée, encre, banc), en nanosecondes.
inline std::int64_t monotonicNowNs() noexcept
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

} // namespace actcraft::core
