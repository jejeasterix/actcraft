#include <actcraft/features/countdown.h>

#include <QRegularExpression>

#include <algorithm>

namespace actcraft::features {

namespace {
constexpr std::int64_t kSecondNs = 1'000'000'000;
} // namespace

void Countdown::start(int seconds, std::int64_t nowNs)
{
    m_durationNs = static_cast<std::int64_t>(std::max(0, seconds)) * kSecondNs;
    m_endNs = nowNs + m_durationNs;
    m_pausedRemainingNs = m_durationNs;
    m_running = true;
    m_paused = false;
}

void Countdown::pause(std::int64_t nowNs)
{
    if (!m_running || m_paused)
        return;
    m_pausedRemainingNs = std::max<std::int64_t>(0, m_endNs - nowNs);
    m_paused = true;
}

void Countdown::resume(std::int64_t nowNs)
{
    if (!m_running || !m_paused)
        return;
    m_endNs = nowNs + m_pausedRemainingNs;
    m_paused = false;
}

void Countdown::reset(std::int64_t nowNs)
{
    m_endNs = nowNs + m_durationNs;
    m_pausedRemainingNs = m_durationNs;
}

void Countdown::setRepeat(bool repeat) noexcept
{
    m_repeat = repeat;
}

bool Countdown::repeat() const noexcept
{
    return m_repeat;
}

bool Countdown::running() const noexcept
{
    return m_running;
}

bool Countdown::paused() const noexcept
{
    return m_paused;
}

int Countdown::remainingSeconds(std::int64_t nowNs) const noexcept
{
    const std::int64_t rest = m_paused ? m_pausedRemainingNs : std::max<std::int64_t>(0, m_endNs - nowNs);
    return static_cast<int>((rest + kSecondNs - 1) / kSecondNs);
}

bool Countdown::poll(std::int64_t nowNs)
{
    if (!m_running || m_paused || nowNs < m_endNs)
        return false;
    if (m_repeat && m_durationNs > 0) {
        while (m_endNs <= nowNs)
            m_endNs += m_durationNs;
    } else {
        m_running = false;
    }
    return true;
}

void Stopwatch::start(std::int64_t nowNs)
{
    m_startNs = nowNs;
    m_pausedElapsedNs = 0;
    m_running = true;
    m_paused = false;
}

void Stopwatch::pause(std::int64_t nowNs)
{
    if (!m_running || m_paused)
        return;
    m_pausedElapsedNs = std::max<std::int64_t>(0, nowNs - m_startNs);
    m_paused = true;
}

void Stopwatch::resume(std::int64_t nowNs)
{
    if (!m_running || !m_paused)
        return;
    m_startNs = nowNs - m_pausedElapsedNs;
    m_paused = false;
}

int Stopwatch::elapsedSeconds(std::int64_t nowNs) const noexcept
{
    if (!m_running)
        return 0;
    const std::int64_t elapsed = m_paused ? m_pausedElapsedNs : std::max<std::int64_t>(0, nowNs - m_startNs);
    return static_cast<int>(elapsed / kSecondNs);
}

QString formatHms(int seconds)
{
    const int s = std::max(0, seconds);
    return QStringLiteral("%1:%2:%3")
        .arg(s / 3600, 2, 10, QLatin1Char('0'))
        .arg(s / 60 % 60, 2, 10, QLatin1Char('0'))
        .arg(s % 60, 2, 10, QLatin1Char('0'));
}

int parseHms(const QString& text)
{
    static const QRegularExpression re(QStringLiteral("^(\\d{1,2}):([0-5]\\d):([0-5]\\d)$"));
    const QRegularExpressionMatch m = re.match(text);
    if (!m.hasMatch())
        return -1;
    return m.captured(1).toInt() * 3600 + m.captured(2).toInt() * 60 + m.captured(3).toInt();
}

} // namespace actcraft::features
