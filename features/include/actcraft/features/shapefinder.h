#pragma once

#include <QPointF>
#include <QRectF>

#include <optional>
#include <vector>

namespace actcraft::features {

struct RecognizedShape
{
    enum class Kind
    {
        Line,
        Rectangle,
        Square,
        Ellipse,
        Circle,
        Triangle,
        Polygon
    };
    Kind kind = Kind::Line;
    QRectF bounds; // non tourné, dans le repère de la forme (centre = centre de la forme)
    double angleDegrees =
        0; // rotation autour du centre de `bounds`, sens de core::rotation (horaire à l'écran)
    // Sommets en coordonnées de page : triangle et polygone (dans l'ordre du tracé) ; rectangle et carré
    // (coins de `bounds` tourné : haut gauche, haut droit, bas droit, bas gauche) ; extrémités (ligne).
    std::vector<QPointF> corners;
};

// Reconnaissance de forme (F-296), algorithme à nous ; rien si le trait ne ressemble à aucune forme.
std::optional<RecognizedShape> recognizeShape(const std::vector<QPointF>& points);

} // namespace actcraft::features
