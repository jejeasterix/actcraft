#pragma once

// Scénarios rejoués par le banc (décision 22, durées N2-1) : tracé, zoom, défilement, et trait de
// démonstration.

#include <QPoint>
#include <QPointF>
#include <QString>
#include <Qt>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace actcraft::bench {

enum class InputKind : std::uint8_t
{
    TabletPress,
    TabletMove,
    TabletRelease,
    Wheel,
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
};

enum class ScenarioSet
{
    Complet,
    Court,
};

struct ScenarioPlan
{
    QString name;         // stroke | zoom | scroll
    double zoom = 1.0;    // niveau imposé avant la mesure (décision 17)
    bool topLeft = false; // page calée en haut à gauche (défilement) ; sinon position par défaut
    std::vector<PlannedInput> inputs;
};

inline constexpr std::int64_t kStylusPeriodNs = 5'000'000; // 200 points/s
inline constexpr std::int64_t kWheelPeriodNs = 8'333'333;  // 120 Hz
inline constexpr std::size_t kDemoMidIndex = 60;           // capture « mi-trait » après 60 événements

int durationDivisor(ScenarioSet set) noexcept; // 1 (complet) ou 5 (court)
ScenarioPlan strokeScenario(ScenarioSet set);
ScenarioPlan zoomScenario(ScenarioSet set);
ScenarioPlan scrollScenario(ScenarioSet set);
std::vector<ScenarioPlan> allScenarios(ScenarioSet set); // stroke, zoom, scroll
// Trait de démonstration non mesuré, bande y = 670 (décision 31).
std::vector<PlannedInput> demoStroke();
// Point du trait de démonstration déjà tracé à la capture « mi-trait ».
QPointF demoInkProbePoint();
// `count` deltas entiers dont la somme vaut exactement `total`.
std::vector<int> cumulativeSplit(int total, int count);
std::int64_t planDurationNs(const ScenarioPlan& plan);

} // namespace actcraft::bench
