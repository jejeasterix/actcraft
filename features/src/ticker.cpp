#include "actcraft/features/ticker.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QtGlobal>

#include <algorithm>

namespace actcraft::features {

namespace {

constexpr int kMinSpeed = 1;
constexpr int kMaxSpeed = 10;
constexpr double kPixelsPerSpeedStep = 40.0;

const QString kFormat = QStringLiteral("actcraft-ticker/1");

QString colorToText(const QColor& color)
{
    return color.name(QColor::HexArgb);
}

QColor colorFromText(const QJsonValue& value, const QColor& fallback)
{
    if (!value.isString()) {
        return fallback;
    }
    const QColor color = QColor::fromString(value.toString());
    return color.isValid() ? color : fallback;
}

} // namespace

double TickerSettings::pixelsPerSecond(int speed) noexcept
{
    return kPixelsPerSpeedStep * std::clamp(speed, kMinSpeed, kMaxSpeed);
}

TickerSettings::Position TickerSettings::positionFromText(const QString& text) noexcept
{
    if (text == QLatin1String("center")) {
        return Position::Center;
    }
    if (text == QLatin1String("bottom")) {
        return Position::Bottom;
    }
    return Position::Top;
}

QString TickerSettings::positionToText(Position position)
{
    switch (position) {
    case Position::Center:
        return QStringLiteral("center");
    case Position::Bottom:
        return QStringLiteral("bottom");
    case Position::Top:
        break;
    }
    return QStringLiteral("top");
}

TickerRun::TickerRun(double screenWidth, double textWidth, double pixelsPerSecond, bool loop)
    : m_screenWidth(screenWidth), m_pixelsPerSecond(pixelsPerSecond), m_loop(loop), m_durationMs(0)
{
    if (pixelsPerSecond > 0.0) {
        m_durationMs = qRound((screenWidth + textWidth) / pixelsPerSecond * 1000.0);
    }
}

int TickerRun::durationMs() const noexcept
{
    return m_durationMs;
}

double TickerRun::xAt(std::int64_t elapsedMs) const noexcept
{
    std::int64_t t = std::max<std::int64_t>(elapsedMs, 0);
    if (m_durationMs <= 0) {
        t = 0;
    } else if (m_loop) {
        t %= m_durationMs;
    } else {
        t = std::min<std::int64_t>(t, m_durationMs);
    }
    return m_screenWidth - m_pixelsPerSecond * static_cast<double>(t) / 1000.0;
}

bool TickerRun::finishedAt(std::int64_t elapsedMs) const noexcept
{
    return !m_loop && elapsedMs >= m_durationMs;
}

QByteArray tickerToJson(const TickerSettings& settings)
{
    QJsonObject o;
    o.insert(QStringLiteral("format"), kFormat);
    o.insert(QStringLiteral("text"), settings.text);
    o.insert(QStringLiteral("fontFamily"), settings.fontFamily);
    o.insert(QStringLiteral("fontPointSize"), settings.fontPointSize);
    o.insert(QStringLiteral("textColor"), colorToText(settings.textColor));
    o.insert(QStringLiteral("shadowOffset"), settings.shadowOffset);
    o.insert(QStringLiteral("shadowColor"), colorToText(settings.shadowColor));
    o.insert(QStringLiteral("screenBackground"), settings.screenBackground);
    o.insert(QStringLiteral("backgroundColor"), colorToText(settings.backgroundColor));
    o.insert(QStringLiteral("position"), TickerSettings::positionToText(settings.position));
    o.insert(QStringLiteral("loop"), settings.loop);
    o.insert(QStringLiteral("speed"), settings.speed);
    return QJsonDocument(o).toJson(QJsonDocument::Indented);
}

std::optional<TickerSettings> tickerFromJson(const QByteArray& data)
{
    if (data.size() > kMaxTickerFileBytes) {
        return std::nullopt; // refusé avant toute analyse
    }
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return std::nullopt;
    }
    const QJsonObject o = doc.object();
    if (o.value(QStringLiteral("format")).toString() != kFormat) {
        return std::nullopt;
    }

    TickerSettings s;
    s.text = o.value(QStringLiteral("text")).toString(s.text);
    s.fontFamily = o.value(QStringLiteral("fontFamily")).toString(s.fontFamily);
    s.fontPointSize = o.value(QStringLiteral("fontPointSize")).toDouble(s.fontPointSize);
    s.textColor = colorFromText(o.value(QStringLiteral("textColor")), s.textColor);
    s.shadowOffset = o.value(QStringLiteral("shadowOffset")).toInt(s.shadowOffset);
    s.shadowColor = colorFromText(o.value(QStringLiteral("shadowColor")), s.shadowColor);
    s.screenBackground = o.value(QStringLiteral("screenBackground")).toBool(s.screenBackground);
    s.backgroundColor = colorFromText(o.value(QStringLiteral("backgroundColor")), s.backgroundColor);
    s.position = TickerSettings::positionFromText(o.value(QStringLiteral("position")).toString());
    s.loop = o.value(QStringLiteral("loop")).toBool(s.loop);
    s.speed = std::clamp(o.value(QStringLiteral("speed")).toInt(s.speed), kMinSpeed, kMaxSpeed);
    return s;
}

} // namespace actcraft::features
