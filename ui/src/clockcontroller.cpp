#include "clockcontroller.h"

#include <actcraft/core/clock.h>

#include <QDateTime>
#include <QVariantMap>

#include <algorithm>

namespace actcraft::ui {

namespace {

constexpr int kTickMs = 250;
constexpr int kMaxSeconds = 99 * 3600 + 59 * 60 + 59;
constexpr std::int64_t kMillisecondNs = 1'000'000;

QVariantMap action(const QString& command, const QString& label)
{
    return {{QStringLiteral("command"), command}, {QStringLiteral("label"), label}};
}

} // namespace

ClockController::ClockController(QObject* parent) : QObject(parent)
{
    m_timer.setInterval(kTickMs);
    connect(&m_timer, &QTimer::timeout, this, &ClockController::tick);
}

int ClockController::source() const noexcept
{
    return m_source;
}

void ClockController::setSource(int source)
{
    if (source < Clock || source > Countdown || source == m_source) {
        return;
    }
    const bool wasPaused = paused();
    m_source = source;
    if (source == Stopwatch && !m_stopwatchStarted) {
        m_stopwatch.start(nowNs());
        m_stopwatchStarted = true;
    }
    emit sourceChanged();
    if (paused() != wasPaused) {
        emit pausedChanged();
    }
    m_lastDisplay.clear();
    tick();
}

int ClockController::mode() const noexcept
{
    return m_mode;
}

void ClockController::setMode(int mode)
{
    if (mode < Analog || mode > Both || mode == m_mode) {
        return;
    }
    m_mode = mode;
    emit modeChanged();
}

bool ClockController::paused() const noexcept
{
    switch (m_source) {
    case Stopwatch:
        return m_stopwatchPaused;
    case Countdown:
        return m_countdown.paused();
    default:
        return m_clockPaused;
    }
}

bool ClockController::active() const noexcept
{
    return m_active;
}

void ClockController::setActive(bool active)
{
    if (active == m_active) {
        return;
    }
    m_active = active;
    refreshTimer();
    if (m_active) {
        tick();
    }
    emit activeChanged();
}

int ClockController::countdownSeconds() const noexcept
{
    return m_seconds;
}

void ClockController::setCountdownSeconds(int seconds)
{
    const int clamped = std::clamp(seconds, 0, kMaxSeconds);
    if (clamped == m_seconds) {
        return;
    }
    m_seconds = clamped;
    emit countdownSettingsChanged();
    tick();
}

QString ClockController::countdownText() const
{
    return features::formatHms(m_seconds);
}

bool ClockController::repeat() const noexcept
{
    return m_repeat;
}

void ClockController::setRepeat(bool repeat)
{
    if (repeat == m_repeat) {
        return;
    }
    m_repeat = repeat;
    m_countdown.setRepeat(repeat);
    emit countdownSettingsChanged();
}

QString ClockController::endAction() const
{
    return m_endAction;
}

void ClockController::setEndAction(const QString& command)
{
    if (command == m_endAction) {
        return;
    }
    const QVariantList all = endActions();
    const bool known = std::any_of(all.begin(), all.end(), [&](const QVariant& entry) {
        return entry.toMap().value(QStringLiteral("command")).toString() == command;
    });
    if (!known) {
        return;
    }
    m_endAction = command;
    emit countdownSettingsChanged();
}

QVariantList ClockController::endActions() const
{
    return {action(QString(), tr("Aucune")), action(QStringLiteral("page.next"), tr("Page suivante")),
            action(QStringLiteral("page.previous"), tr("Page précédente")),
            action(QStringLiteral("page.revert"), tr("Rétablir la page"))};
}

QString ClockController::remainingText() const
{
    if (!m_countdownStarted) {
        return features::formatHms(m_seconds);
    }
    return features::formatHms(m_countdown.remainingSeconds(nowNs()));
}

QString ClockController::displayText() const
{
    switch (m_source) {
    case Stopwatch:
        return features::formatHms(m_stopwatch.elapsedSeconds(nowNs()));
    case Countdown:
        return remainingText();
    default:
        return wallTime().toString(QStringLiteral("HH:mm:ss"));
    }
}

qreal ClockController::hourAngle() const
{
    const QTime time = wallTime();
    return (time.hour() % 12) * 30.0 + time.minute() * 0.5;
}

qreal ClockController::minuteAngle() const
{
    const QTime time = wallTime();
    return time.minute() * 6.0 + time.second() * 0.1;
}

qreal ClockController::secondAngle() const
{
    return wallTime().second() * 6.0;
}

qint64 ClockController::clockNowOffsetMs() const noexcept
{
    return m_offsetMs;
}

void ClockController::setClockNowOffsetMs(qint64 milliseconds)
{
    if (milliseconds == m_offsetMs) {
        return;
    }
    m_offsetMs = milliseconds;
    emit displayChanged();
}

Commands* ClockController::commands() const noexcept
{
    return m_commands;
}

void ClockController::setCommands(Commands* commands)
{
    if (commands == m_commands) {
        return;
    }
    m_commands = commands;
    emit commandsChanged();
}

void ClockController::cycleMode()
{
    setMode((m_mode + 1) % 3);
}

void ClockController::cycleSource()
{
    setSource((m_source + 1) % 3);
}

void ClockController::togglePause()
{
    const std::int64_t now = nowNs();
    switch (m_source) {
    case Stopwatch:
        m_stopwatchPaused ? m_stopwatch.resume(now) : m_stopwatch.pause(now);
        m_stopwatchPaused = !m_stopwatchPaused;
        break;
    case Countdown:
        m_countdown.paused() ? m_countdown.resume(now) : m_countdown.pause(now);
        break;
    default:
        if (!m_clockPaused) {
            m_frozenWall = wallTime();
        }
        m_clockPaused = !m_clockPaused;
        break;
    }
    emit pausedChanged();
    m_lastDisplay.clear();
    tick();
}

bool ClockController::applyCountdownText(const QString& text)
{
    const int seconds = features::parseHms(text.trimmed());
    if (seconds <= 0) {
        return false;
    }
    setCountdownSeconds(seconds);
    return true;
}

void ClockController::startCountdown()
{
    if (m_seconds <= 0) {
        return;
    }
    const bool wasPaused = paused();
    m_countdown.start(m_seconds, nowNs());
    m_countdownStarted = true;
    if (m_source != Countdown) {
        m_source = Countdown;
        emit sourceChanged();
    }
    if (paused() != wasPaused) {
        emit pausedChanged();
    }
    m_lastDisplay.clear();
    tick();
}

void ClockController::resetCountdown()
{
    if (m_countdownStarted && m_countdown.running()) {
        m_countdown.reset(nowNs());
        m_lastDisplay.clear();
        tick();
        return;
    }
    startCountdown();
}

void ClockController::stop()
{
    const bool wasPaused = paused();
    m_countdown = features::Countdown();
    m_countdown.setRepeat(m_repeat);
    m_stopwatch = features::Stopwatch();
    m_countdownStarted = false;
    m_stopwatchStarted = false;
    m_stopwatchPaused = false;
    if (m_source == Stopwatch) {
        m_source = Clock;
        emit sourceChanged();
    }
    if (paused() != wasPaused) {
        emit pausedChanged();
    }
    m_lastDisplay.clear();
    emit displayChanged();
    refreshTimer();
}

std::int64_t ClockController::nowNs() const
{
    return core::monotonicNowNs() + m_offsetMs * kMillisecondNs;
}

QTime ClockController::wallTime() const
{
    if (m_clockPaused && m_frozenWall.isValid()) {
        return m_frozenWall;
    }
    return QDateTime::currentDateTime().addMSecs(m_offsetMs).time();
}

void ClockController::tick()
{
    if (m_countdownStarted && m_countdown.poll(nowNs())) {
        emit countdownEnded();
        if (!m_endAction.isEmpty() && m_commands) {
            m_commands->trigger(m_endAction);
        }
    }
    const QString display = displayText() + QLatin1Char('|') + remainingText();
    if (display != m_lastDisplay) {
        m_lastDisplay = display;
        emit displayChanged();
    }
    refreshTimer();
}

// Panneau affiché : la minuterie relit l'heure. Panneau caché (revue qualité z2) : elle tourne encore tant
// qu'un compte à rebours court, pour que sa fin et son action tombent à l'heure et non au retour du panneau.
void ClockController::refreshTimer()
{
    const bool needed = m_active || (m_countdownStarted && m_countdown.running());
    if (needed && !m_timer.isActive()) {
        m_timer.start();
    } else if (!needed && m_timer.isActive()) {
        m_timer.stop();
    }
}

} // namespace actcraft::ui
