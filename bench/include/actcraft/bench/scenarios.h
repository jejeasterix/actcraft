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
    QString name;         // stroke | zoom | scroll | page | pauses | gommer | soulever
    double zoom = 1.0;    // niveau imposé avant la mesure (décision 17)
    bool topLeft = false; // page calée en haut à gauche (défilement) ; sinon position par défaut
    // > 0 : document de `generatedPages` pages générées chargé avant la mesure
    // (DocumentController::loadGeneratedPages, pages paires peintes) ; 0 : la page en place.
    int generatedPages = 0;
    // Relevés propres au scénario « page » (L2-T23) : reconstructions et rastérisations de la page fixe au
    // zoom 150, éclair blanc au changement de page (runner.cpp). Lus par ce booléen, jamais par le nom.
    bool pageChecks = false;
    // L3-T19 : commande nommée d'outil appelée par le joueur avant la mesure (« tool.eraser », « tool.select
    // »), puis « tool.pen » après elle ; vide : l'outil en place (stylo) reste.
    QString toolCommand;
    // Annotations (couche supérieure) posées par le joueur sur la page chargée et versées dans le cache avant
    // la mesure, en points de page. Les traits de la page générée sont sur la couche du milieu : la gomme ne
    // les touche pas, il faut donc des annotations pour que le passage de gomme coupe quelque chose.
    std::vector<std::vector<QPointF>> seedStrokes;
    // Relevés écrits dans le JSON (cache.touched_strokes, cache.pending_cut) ; -1 : non applicable.
    int touchedStrokes = -1; // k : annotations que le passage de gomme touche
    int liftedObjects = -1;  // soulever : objets que le rectangle de sélection prend (2 à 8)
    int pendingCut = -1;     // 1 si le dernier trait encore en attente est gommé en deux morceaux
    // L3-C10 (S36, M3) : retire figements et reprises hors limite du verdict de ce scénario (lu par ce
    // booléen, jamais par le nom) ; copié dans ScenarioResult::freezeExempt.
    bool freezeExempt = false;
    // Attendu S36 sur les redessins du cache (copié dans ScenarioResult::maxCacheRefreshes) ; < 0 : aucun.
    int maxCacheRefreshes = -1;
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
// Lot 10 (DA11) : décalage de la reprise d'un trait à l'autre, après le délai d'inactivité du cache.
inline constexpr std::int64_t kPausesStepNs = 8'000'000;
// Traits séparés par ui::kCacheFoldIdleMs + k × kPausesStepNs (complet : 10 traits de 0,5 s, pause après le
// trait k : k = 0 à 8 ; court : 4 traits de 0,2 s, 3k pour k = 0 à 2) : chaque reprise tombe dans la fenêtre
// du redessin différé du cache. Page du banc rechargée avant (generatedPages = 1 : page de 300 objets, sans
// éléments peints).
ScenarioPlan pausesScenario(ScenarioSet set);
// L3-T19 (H10-8 e, S36). Gomme : 24 annotations horizontales posées et versées avant la mesure, puis un
// zigzag du stylo qui les croise toutes (gomme de largeur réglée par défaut, 50 px de page). Aucun trait
// n'attend au-dessus du cache : pending_cut = 0.
inline constexpr double kEraserRadiusPage = 25.0;
ScenarioPlan eraseScenario(ScenarioSet set);
// Soulèvement : rectangle de sélection qui prend deux objets de la page générée, puis appui sur l'un d'eux et
// glissement de 300 px en 2 s (complet ; ÷ 5 en court). Page du banc rechargée.
ScenarioPlan liftScenario(ScenarioSet set);
// stroke, zoom, scroll, page, pauses, gommer, soulever
std::vector<ScenarioPlan> allScenarios(ScenarioSet set);
// Trait de démonstration non mesuré, bande y = 670 (décision 31).
std::vector<PlannedInput> demoStroke();
// Point du trait de démonstration déjà tracé à la capture « mi-trait ».
QPointF demoInkProbePoint();
// `count` deltas entiers dont la somme vaut exactement `total`.
std::vector<int> cumulativeSplit(int total, int count);
std::int64_t planDurationNs(const ScenarioPlan& plan);
// Scénario « page » (revue L2-T23) : un changement de page dure de sa commande (`first`) jusqu'à la première
// image qui suit le prêt de la page affichée (`second` : images décodées, cache redessiné). Rend, pour chaque
// intervalle d'images (swapNs[k-1], swapNs[k]] qui touche cette fenêtre (dont celui où la commande s'exécute,
// même partie en retard sur son heure prévue), l'instant swapNs[k-1] + 1 : ajoutés aux entrées,
// classifyIntervals compte ces intervalles actifs (décodage, reconstruction, redessin).
std::vector<std::int64_t> pageChangeMarks(const std::vector<std::int64_t>& swapNs,
                                          const std::vector<std::pair<std::int64_t, std::int64_t>>& changes);
// Fenêtres des commandes jouées (`commandNs[i]` ≥ 0) : jusqu'au premier prêt relevé après elles (`shownNs`,
// trié ; aucun : jusqu'à la fin), ou jusqu'à la commande elle-même si elle n'en attend aucun
// (`awaitsReady[i]` faux : page et cache encore prêts juste après elle, comme un zoom sans cache).
std::vector<std::pair<std::int64_t, std::int64_t>>
pageChangeWindows(const std::vector<std::int64_t>& commandNs, const std::vector<bool>& awaitsReady,
                  const std::vector<std::int64_t>& shownNs);

} // namespace actcraft::bench
