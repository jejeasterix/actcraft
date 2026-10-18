#pragma once

#include <actcraft/core/model.h>

#include <optional>
#include <vector>

namespace actcraft::core {

// Désignation (F-210), coordonnées de page. L'appelant passe kHitToleranceScreen / zoom.
inline constexpr double kHitToleranceScreen = 6.0;

// Annotation (F-230) : trait de stylo, de marqueur ou d'encre révélatrice posé sur la couche supérieure.
bool isAnnotation(const PageObject& object) noexcept;
// Sélectionnable : visible (un objet verrouillé se sélectionne mais ne se déplace pas, F-210).
bool isSelectable(const PageObject& object) noexcept;
// Objet sélectionnable le plus haut (le dernier de page.objects, ordonnée par couche) touché par `point` : un
// trait à moins de sa demi-épaisseur + `tolerance` de sa ligne centrale (transformation appliquée) ; un autre
// objet, point dans son emprise (boundsOnPage) agrandie de `tolerance`.
std::optional<ObjectId> objectAt(const Page& page, PointF point, double tolerance);
// Objets sélectionnables dont l'emprise touche `rect` (bords compris), dans l'ordre de la page (D6, H3-4).
std::vector<ObjectId> objectsTouching(const Page& page, const RectF& rect);
RectF normalizedRect(PointF a, PointF b) noexcept;
RectF unitedRect(const RectF& a, const RectF& b) noexcept; // un rectangle 0 × 0 est neutre
bool rectsTouch(const RectF& a, const RectF& b) noexcept;

} // namespace actcraft::core
