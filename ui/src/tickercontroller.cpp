#include "tickercontroller.h"

#include <QSettings>

#include <algorithm>

namespace actcraft::ui {

namespace {

// Réglages gardés en un seul bloc, au format des fichiers de messages (features::tickerToJson) : relus par
// tickerFromJson, qui borne la taille et garde le texte en texte brut.
const char* const kSettingsKey = "tools/ticker/message";

using features::TickerSettings;

// Vitesse ramenée aux bornes de la logique (pixelsPerSecond borne déjà) : aucune borne recopiée ici.
int clampedSpeed(int speed) noexcept
{
    return qRound(TickerSettings::pixelsPerSecond(speed) / TickerSettings::pixelsPerSecond(1));
}

TickerSettings::Position toPosition(int position) noexcept
{
    switch (position) {
    case TickerController::Center:
        return TickerSettings::Position::Center;
    case TickerController::Bottom:
        return TickerSettings::Position::Bottom;
    default:
        return TickerSettings::Position::Top;
    }
}

} // namespace

TickerController::TickerController(QObject* parent) : QObject(parent)
{
    QSettings settings;
    const QByteArray stored = settings.value(QString::fromLatin1(kSettingsKey)).toString().toUtf8();
    if (!stored.isEmpty()) {
        if (const auto read = features::tickerFromJson(stored)) {
            // Par les modificateurs : mêmes contrôles qu'à la saisie, une valeur refusée garde celle de la
            // logique.
            setText(read->text);
            setFontFamily(read->fontFamily);
            setFontPointSize(read->fontPointSize);
            setTextColor(read->textColor);
            setShadowOffset(read->shadowOffset);
            setShadowColor(read->shadowColor);
            setScreenBackground(read->screenBackground);
            setBackgroundColor(read->backgroundColor);
            m_settings.position = read->position;
            setLoop(read->loop);
            setSpeed(read->speed);
        }
    }
}

QString TickerController::text() const
{
    return m_settings.text;
}

void TickerController::setText(const QString& text)
{
    if (m_settings.text == text) {
        return;
    }
    m_settings.text = text;
    emit settingsChanged();
}

QString TickerController::fontFamily() const
{
    return m_settings.fontFamily;
}

void TickerController::setFontFamily(const QString& family)
{
    if (m_settings.fontFamily == family) {
        return;
    }
    m_settings.fontFamily = family;
    emit settingsChanged();
}

qreal TickerController::fontPointSize() const noexcept
{
    return m_settings.fontPointSize;
}

void TickerController::setFontPointSize(qreal size)
{
    if (!(size > 0) || qFuzzyCompare(m_settings.fontPointSize, size)) {
        return;
    }
    m_settings.fontPointSize = size;
    emit settingsChanged();
}

QColor TickerController::textColor() const noexcept
{
    return m_settings.textColor;
}

void TickerController::setTextColor(const QColor& color)
{
    if (!color.isValid() || m_settings.textColor == color) {
        return;
    }
    m_settings.textColor = color;
    emit settingsChanged();
}

int TickerController::shadowOffset() const noexcept
{
    return m_settings.shadowOffset;
}

void TickerController::setShadowOffset(int offset)
{
    offset = std::max(offset, 0);
    if (m_settings.shadowOffset == offset) {
        return;
    }
    m_settings.shadowOffset = offset;
    emit settingsChanged();
}

QColor TickerController::shadowColor() const noexcept
{
    return m_settings.shadowColor;
}

void TickerController::setShadowColor(const QColor& color)
{
    if (!color.isValid() || m_settings.shadowColor == color) {
        return;
    }
    m_settings.shadowColor = color;
    emit settingsChanged();
}

bool TickerController::screenBackground() const noexcept
{
    return m_settings.screenBackground;
}

void TickerController::setScreenBackground(bool screen)
{
    if (m_settings.screenBackground == screen) {
        return;
    }
    m_settings.screenBackground = screen;
    emit settingsChanged();
}

QColor TickerController::backgroundColor() const noexcept
{
    return m_settings.backgroundColor;
}

void TickerController::setBackgroundColor(const QColor& color)
{
    if (!color.isValid() || m_settings.backgroundColor == color) {
        return;
    }
    m_settings.backgroundColor = color;
    emit settingsChanged();
}

int TickerController::position() const noexcept
{
    switch (m_settings.position) {
    case TickerSettings::Position::Center:
        return Center;
    case TickerSettings::Position::Bottom:
        return Bottom;
    case TickerSettings::Position::Top:
        break;
    }
    return Top;
}

void TickerController::setPosition(int position)
{
    const TickerSettings::Position where = toPosition(position);
    if (m_settings.position == where) {
        return;
    }
    m_settings.position = where;
    emit settingsChanged();
}

bool TickerController::loop() const noexcept
{
    return m_settings.loop;
}

void TickerController::setLoop(bool loop)
{
    if (m_settings.loop == loop) {
        return;
    }
    m_settings.loop = loop;
    emit settingsChanged();
}

int TickerController::speed() const noexcept
{
    return m_settings.speed;
}

void TickerController::setSpeed(int speed)
{
    speed = clampedSpeed(speed);
    if (m_settings.speed == speed) {
        return;
    }
    m_settings.speed = speed;
    emit settingsChanged();
}

qreal TickerController::pixelsPerSecond() const noexcept
{
    return TickerSettings::pixelsPerSecond(m_settings.speed);
}

bool TickerController::running() const noexcept
{
    return m_running;
}

bool TickerController::paused() const noexcept
{
    return m_paused;
}

const features::TickerSettings& TickerController::settings() const noexcept
{
    return m_settings;
}

void TickerController::start()
{
    setPaused(false);
    if (m_running) {
        // Relance : la surcouche repart du bord droit (running repasse par faux).
        m_running = false;
        emit runningChanged();
    }
    m_running = true;
    emit runningChanged();
}

void TickerController::stop()
{
    setPaused(false);
    if (!m_running) {
        return;
    }
    m_running = false;
    emit runningChanged();
}

void TickerController::togglePause()
{
    if (!m_running) {
        return;
    }
    setPaused(!m_paused);
}

void TickerController::finished()
{
    if (!m_settings.loop) {
        stop();
    }
}

int TickerController::durationMs(qreal screenWidth, qreal textWidth) const
{
    const features::TickerRun run(screenWidth, textWidth, pixelsPerSecond(), m_settings.loop);
    return std::max(run.durationMs(), 1);
}

qreal TickerController::positionAt(qreal screenWidth, qreal textWidth, qreal speedPx, double elapsedMs) const
{
    const features::TickerRun run(screenWidth, textWidth, speedPx, m_settings.loop);
    return run.xAt(static_cast<std::int64_t>(elapsedMs));
}

void TickerController::keepAsDefault() const
{
    QSettings settings;
    settings.setValue(QString::fromLatin1(kSettingsKey),
                      QString::fromUtf8(features::tickerToJson(m_settings)));
}

void TickerController::setPaused(bool paused)
{
    if (m_paused == paused) {
        return;
    }
    m_paused = paused;
    emit pausedChanged();
}

} // namespace actcraft::ui
