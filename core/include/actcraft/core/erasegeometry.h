#pragma once

#include <actcraft/core/model.h>

#include <vector>

namespace actcraft::core {

struct StrokeCut
{
    std::vector<std::vector<StrokePoint>>
        pieces;           // morceaux restants (≥ 2 points chacun), dans l'ordre du trait
    bool touched = false; // le disque a retiré une partie du trait
};

// Gomme (F-230) : retire de la ligne centrale la partie intérieure au disque (`center`, `radius`) ; pression
// interpolée aux points de coupe ; un reste d'un seul point est abandonné.
StrokeCut cutByDisk(const std::vector<StrokePoint>& points, PointF center, double radius);
// Disque glissé de `from` à `to` : coupes successives à des pas d'au plus radius / 2 (aucun trait sauté).
// Aucun centre n'est appliqué deux fois dans un même appel : un geste immobile (`from == to`) n'applique
// qu'un centre. Pour enchaîner a -> b puis b -> c, passer `includeFrom = false` au second appel : le centre
// `b` a déjà été appliqué par le premier (le rejouer mangerait un morceau dont le bout est pile sur le
// cercle).
StrokeCut cutBySweep(const std::vector<StrokePoint>& points, PointF from, PointF to, double radius,
                     bool includeFrom = true);
// Points d'un objet trait en coordonnées de page (sa transformation appliquée) ; vide si ce n'est pas un
// trait.
std::vector<StrokePoint> strokePointsOnPage(const PageObject& object);

} // namespace actcraft::core
