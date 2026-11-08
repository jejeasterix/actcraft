#pragma once

#include <QPointF>

#include <array>
#include <cmath>
#include <optional>

namespace actcraft::features {

enum class Side
{
    Left,
    Right,
    Top,
    Bottom
};

// Bord du cadre (left, top, right, bottom) le plus proche de `point`, à `margin` près ; rien au-delà. Un bord
// ne compte que si le point est à sa hauteur (ou à sa largeur) : dans le cadre élargi de `margin` le long du
// bord. À distance égale, le bord qui vient le premier dans `order` l'emporte.
inline std::optional<Side> nearestSide(QPointF point, double left, double top, double right, double bottom,
                                       double margin, const std::array<Side, 4>& order)
{
    const bool alongX = point.x() >= left - margin && point.x() <= right + margin;
    const bool alongY = point.y() >= top - margin && point.y() <= bottom + margin;
    const auto distance = [&](Side side) {
        switch (side) {
        case Side::Left:
            return alongY ? std::abs(point.x() - left) : margin + 1;
        case Side::Right:
            return alongY ? std::abs(point.x() - right) : margin + 1;
        case Side::Top:
            return alongX ? std::abs(point.y() - top) : margin + 1;
        case Side::Bottom:
            return alongX ? std::abs(point.y() - bottom) : margin + 1;
        }
        return margin + 1;
    };
    Side best = order[0];
    double bestDistance = distance(best);
    for (std::size_t i = 1; i < order.size(); ++i) {
        const double d = distance(order[i]);
        if (d < bestDistance) {
            best = order[i];
            bestDistance = d;
        }
    }
    if (bestDistance <= margin)
        return best;
    return std::nullopt;
}

} // namespace actcraft::features
