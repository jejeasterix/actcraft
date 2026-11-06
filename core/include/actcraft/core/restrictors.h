#pragma once

#include <actcraft/core/model.h>

#include <vector>

// Limiteurs et rotation (lot 8, F-578, F-579, E6, D8-7) : logique pure, sans modification du document. Toute
// géométrie est en coordonnées de page : la boîte d'un objet est celle de son emprise passée par sa
// transformation. Les booléens du lot 2 (`movable`, `resizable`, `rotatable`) restent la source du
// « bloqué » : un objet lu bloqué d'un fichier garde ses modes par défaut et ne bouge pas pour autant.
namespace actcraft::core {

// Déplacement permis de l'objet, depuis sa position de départ, pour un déplacement voulu `wanted` (page).
PointF allowedTravel(const Page& page, const PageObject& object, PointF wanted);
// Point du chemin (Travel::AlongTrack) le plus proche de `point` ; `point` si pas de chemin.
PointF projectOnTrack(const Page& page, const PageObject& object, PointF point);
// Position du centre après accrochage (magnet), ou `dropped` si l'objet n'accroche pas. Le repère de l'objet
// se pose sur le même repère du conteneur le plus haut sous `dropped` ; Mark::Point : le centre se pose sur
// `magnetPoint` (page). Aucune règle d'acceptation (containers.h, L8-T14) : le choix du conteneur revient à
// l'appelant (L8-T16), qui n'accroche que si ce conteneur accepte l'objet.
PointF landingPoint(const Page& page, const PageObject& object, PointF dropped);
bool sizeHandlesShown(const PageObject& object) noexcept; // faux si dimension bloquée (E6)
// Angle permis pour l'angle voulu `wantedDegrees` (sens horaire positif). Contrat : `fromDegrees` est le
// dernier angle permis, celui que cette fonction a rendu au mouvement précédent du même glisser (l'angle de
// l'objet au premier), jamais l'angle de départ du glisser : le sens se lit sur le tour le plus court entre
// les deux. L'angle voulu peut être ramené à ]-180, 180] ; le résultat reste continu avec `fromDegrees`.
double allowedTurn(const PageObject& object, double fromDegrees, double wantedDegrees) noexcept;
PointF pivotOf(const Page& page, const PageObject& object); // page
// Déplacement réduit pour que la boîte des objets `moving` ne passe sur aucun objet bloquant.
PointF stopAtBlockers(const Page& page, const std::vector<ObjectId>& moving, PointF wanted);

} // namespace actcraft::core
