#include <actcraft/features/recognitionsession.h>

#include <algorithm>

namespace actcraft::features {

RecognitionSession::RecognitionSession(int delayMs)
{
    setDelayMs(delayMs);
}

void RecognitionSession::setDelayMs(int delayMs) noexcept
{
    m_delayMs = std::clamp(delayMs, 0, kMaxDelayMs);
}

int RecognitionSession::delayMs() const noexcept
{
    return m_delayMs;
}

void RecognitionSession::addStroke(core::ObjectId id, std::int64_t nowNs)
{
    m_pending.push_back(id);
    m_lastNs = nowNs;
}

std::vector<core::ObjectId> RecognitionSession::due(std::int64_t nowNs)
{
    // Délai en nanosecondes sur 64 bits : 5000 ms × 1 000 000 dépasse un int.
    if (m_pending.empty() || nowNs - m_lastNs < std::int64_t{m_delayMs} * 1'000'000) {
        return {};
    }
    std::vector<core::ObjectId> ready;
    ready.swap(m_pending);
    return ready;
}

const std::vector<core::ObjectId>& RecognitionSession::pending() const noexcept
{
    return m_pending;
}

void RecognitionSession::clear() noexcept
{
    m_pending.clear();
}

} // namespace actcraft::features
