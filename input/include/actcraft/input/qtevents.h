#pragma once

#include <actcraft/input/sample.h>

#include <QPointF>

#include <cstdint>
#include <optional>

class QEvent;

namespace actcraft::input {

// Tablette (appui, déplacement en contact, relâché), souris bouton gauche (N2-12) ou un seul doigt
// (décision 25) ; nullopt pour le survol, les autres boutons, la souris synthétisée depuis un stylet ou un
// écran tactile, deux doigts ou plus, l'annulation tactile et tout autre événement. `pagePos` : position déjà
// ramenée en coordonnées de page par la vue.
std::optional<Sample> sampleFromEvent(const QEvent& event, QPointF pagePos, std::int64_t nowNs);

} // namespace actcraft::input
