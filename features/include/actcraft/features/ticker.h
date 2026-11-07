#pragma once

#include <QByteArray>
#include <QColor>
#include <QString>

#include <cstdint>
#include <optional>

namespace actcraft::features {

// Réglages du message défilant (F-291). Le texte est du texte brut, jamais interprété.
struct TickerSettings
{
    enum class Position
    {
        Top,
        Center,
        Bottom
    };

    QString text = QStringLiteral("Votre message ici");
    QString fontFamily; // vide : police de l'interface
    double fontPointSize = 32;
    QColor textColor = QColor(0x1F, 0x29, 0x37);
    int shadowOffset = 0; // pixels ; 0 : sans ombre
    QColor shadowColor = QColor(0x00, 0x00, 0x00);
    bool screenBackground = true; // vrai : fond transparent sur l'écran ; faux : backgroundColor
    QColor backgroundColor = QColor(0xFF, 0xF4, 0xC2);
    Position position = Position::Top;
    bool loop = false;
    int speed = 5; // 1 à 10

    static double pixelsPerSecond(int speed) noexcept;              // 40 × vitesse bornée à 1..10
    static Position positionFromText(const QString& text) noexcept; // "top", "center", "bottom" ; sinon Top
    static QString positionToText(Position position);

    friend bool operator==(const TickerSettings&, const TickerSettings&) = default;
};

// Un passage du message : abscisse du bord gauche du texte en fonction du temps écoulé (ms).
class TickerRun
{
public:
    TickerRun(double screenWidth, double textWidth, double pixelsPerSecond, bool loop);
    int durationMs() const noexcept; // (écran + texte) / vitesse
    double
    xAt(std::int64_t elapsedMs) const noexcept; // de screenWidth à -textWidth ; en boucle, modulo la durée
    bool finishedAt(std::int64_t elapsedMs) const noexcept; // jamais en boucle

private:
    double m_screenWidth;
    double m_pixelsPerSecond;
    bool m_loop;
    int m_durationMs;
};

inline constexpr qsizetype kMaxTickerFileBytes = 64 * 1024;
QByteArray tickerToJson(const TickerSettings& settings); // format « actcraft-ticker/1 »
std::optional<TickerSettings>
tickerFromJson(const QByteArray& data); // rien si trop gros, illisible ou autre format

} // namespace actcraft::features
