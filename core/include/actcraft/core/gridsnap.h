#pragma once

#include <actcraft/core/model.h>

#include <optional>
#include <vector>

namespace actcraft::core {

// Intersection la plus proche de `point` parmi les niveaux qui accrochent ; std::nullopt si la grille
// n'autorise pas l'accrochage (snapEnabled faux) ou si aucun niveau n'accroche. Grille invisible : accroche
// quand même.
std::optional<PointF> nearestGridPoint(const Grid& grid, PointF point, double pageWidth,
                                       double pageHeight) noexcept;

// Point d'ancrage d'un objet pour l'accrochage (F-622), en coordonnées de page : ligne de base d'un texte
// (core::textBaseline), sinon coin inférieur gauche de son emprise.
PointF snapAnchor(const PageObject& object);

// Déplacement corrigé pour que l'ancrage du premier objet de `ids` (ordre de la page) tombe, après `delta`,
// sur l'intersection la plus proche ; `delta` inchangé sans accrochage ou sans objet.
PointF snapDelta(const Page& page, const std::vector<ObjectId>& ids, PointF delta) noexcept;

} // namespace actcraft::core
