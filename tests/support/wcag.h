#pragma once

// Contrastes WCAG 2.1 (L11-T24) : luminance relative et rapport de contraste. Copie de ce que calcule
// tst_ui_shell (qui garde la sienne) ; sans dépendance au moteur QML.

#include <QColor>
#include <QHash>
#include <QImage>
#include <QRect>

#include <algorithm>
#include <cmath>

namespace wcag {

constexpr double kText = 4.5; // texte normal
constexpr double kUi = 3.0;   // grand texte, bordures et indicateurs

inline double relativeLuminance(const QColor& color)
{
    const auto linear = [](double c) {
        return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) + 0.0722 * linear(color.blueF());
}

inline double contrastRatio(const QColor& a, const QColor& b)
{
    const double la = relativeLuminance(a);
    const double lb = relativeLuminance(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

// Couleur la plus fréquente d'une zone de l'image : le fond sous un texte (le texte n'en couvre que peu).
inline QColor dominantColor(const QImage& image, const QRect& zone)
{
    const QRect area = zone.intersected(image.rect());
    QHash<QRgb, int> counts;
    QRgb best = 0;
    int bestCount = 0;
    for (int y = area.top(); y <= area.bottom(); ++y) {
        for (int x = area.left(); x <= area.right(); ++x) {
            const QRgb rgb = image.pixel(x, y);
            const int count = ++counts[rgb];
            if (count > bestCount) {
                bestCount = count;
                best = rgb;
            }
        }
    }
    return bestCount > 0 ? QColor::fromRgb(best) : QColor();
}

} // namespace wcag
