#pragma once

// Scénarios rejoués par le banc (décision 22, durées N2-1) : tracé, zoom, défilement, changement de page
// (L2-T23), et trait de démonstration.

#include <QPoint>
#include <QPointF>
#include <QString>
#include <QVariant>
#include <Qt>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace actcraft::bench {

enum class InputKind : std::uint8_t
{
    TabletPress,
    TabletMove,
    TabletRelease,
    Wheel,
    Command, // commande nommée appelée par runCommand de la fenêtre : aucun événement posté (L2-T23)
};

struct PlannedInput
{
    std::int64_t offsetNs = 0; // depuis le début du scénario
    InputKind kind = InputKind::TabletMove;
    QPointF pagePos; // tablette : coordonnées de page, converties en scène au moment de poster
    float pressure = 0.5f;
    QPoint angleDelta; // molette
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    bool measured = false; // compte dans la latence d'encre (appui et déplacements du tracé)
    QString command;       // InputKind::Command : commande nommée appelée par runCommand
    QVariant argument;     // son argument éventuel
};

enum class ScenarioSet
{
    Complet,
    Court,
};

struct ScenarioPlan
{
    QString name;         // stroke | zoom | scroll | page
    double zoom = 1.0;    // niveau imposé avant la mesure (décision 17)
    bool topLeft = false; // page calée en haut à gauche (défilement) ; sinon position par défaut
    // > 0 : document de `generatedPages` pages générées chargé avant la mesure
    // (DocumentController::loadGeneratedPages, pages paires peintes) ; 0 : la page en place.
    int generatedPages = 0;
    std::vector<PlannedInput> inputs;
};

inline constexpr std::int64_t kStylusPeriodNs = 5'000'000; // 200 points/s
inline constexpr std::int64_t kWheelPeriodNs = 8'333'333;  // 120 Hz
inline constexpr std::size_t kDemoMidIndex = 60;           // capture « mi-trait » après 60 événements

int durationDivisor(ScenarioSet set) noexcept; // 1 (complet) ou 5 (court)
ScenarioPlan strokeScenario(ScenarioSet set);
ScenarioPlan zoomScenario(ScenarioSet set);
ScenarioPlan scrollScenario(ScenarioSet set);
// Changements de page à 500 ms d'écart (complet : 20, court : 4) en alternance page.next / page.previous,
// puis page.next, view.zoom 150 et view.fit à 700 ms d'écart sur la page peinte (L2-T23).
ScenarioPlan pageScenario(ScenarioSet set);
std::vector<ScenarioPlan> allScenarios(ScenarioSet set); // stroke, zoom, scroll, page
// Trait de démonstration non mesuré, bande y = 670 (décision 31).
std::vector<PlannedInput> demoStroke();
// Point du trait de démonstration déjà tracé à la capture « mi-trait ».
QPointF demoInkProbePoint();
// `count` deltas entiers dont la somme vaut exactement `total`.
std::vector<int> cumulativeSplit(int total, int count);
std::int64_t planDurationNs(const ScenarioPlan& plan);
// Scénario « page » (revue L2-T23) : un changement de page dure de sa commande (`first`) jusqu'à la première
// image qui suit le prêt de la page affichée (`second` : images décodées, cache redessiné). Rend, pour chaque
// intervalle d'images (swapNs[k-1], swapNs[k]] qui commence dans cette fenêtre, l'instant swapNs[k-1] + 1 :
// ajoutés aux entrées, classifyIntervals compte ces intervalles actifs (décodage, reconstruction, redessin).
std::vector<std::int64_t> pageChangeMarks(const std::vector<std::int64_t>& swapNs,
                                          const std::vector<std::pair<std::int64_t, std::int64_t>>& changes);

} // namespace actcraft::bench
