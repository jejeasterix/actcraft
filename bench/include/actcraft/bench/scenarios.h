#pragma once

// Scénarios rejoués par le banc (décision 22, durées N2-1) : tracé, zoom, défilement, changement de page
// (L2-T23), et trait de démonstration.

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>

#include <QPoint>
#include <QPointF>
#include <QSize>
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
    // L3-C14 : la mesure se ferme sur le versement différé du dernier trait (runner.cpp, Runner::play) : le
    // joueur l'attend après la dernière entrée, rien ne le repousse. Lu par ce booléen, jamais par le nom.
    bool awaitFinalFold = false;
    // L4-T14. Commandes nommées jouées avant la mesure, avant `toolCommand` (« shape.choose » « rectangle » :
    // le préréglage de l'outil des formes).
    std::vector<std::pair<QString, QVariant>> setupCommands;
    // La page chargée reçoit `gridAboveFixture()` (SetGridCommand par le document, versée dans le cache avant
    // la mesure) : page à grille dessinée au-dessus des objets.
    bool gridAbove = false;
    // L4-T28 : la page chargée reçoit 40 x 53 cadres (`kBenchZonesAcross` x `kBenchZonesDown`,
    // SetPageZonesCommand par le document) : séparations des zones dessinées en surcouche pendant la mesure
    // (« zones-zoom »).
    bool zones = false;
    // Formes posées par les glissés du plan (k) ; -1 : sans objet. Écrit dans le JSON (cache.placed_shapes).
    int placedShapes = -1;
    // Relève, dans le JSON, les trames pendant les glissés (clé shape_preview : coût de l'aperçu des formes).
    // Lu par ce booléen, jamais par le nom.
    bool measuresShapePreview = false;
    // L4-T29 (S7.5). Commandes nommées jouées après la mesure (« view.browsers » referme le panneau ouvert
    // avant elle).
    std::vector<std::pair<QString, QVariant>> teardownCommands;
    // Relève, dans le JSON, les miniatures des pages (clé thumbnails : rendus dans la fenêtre mesurée, rendus
    // lancés pendant un contact, copie la plus longue). Lu par ce booléen, jamais par le nom.
    bool measuresThumbnails = false;
    // Le panneau des navigateurs est ouvert pendant la mesure : le banc attend les miniatures à jour avant la
    // mesure et le rendu d'une miniature dans la fenêtre ; le runner vérifie l'état réel du panneau.
    bool browsersOpen = false;
    // L5-T19 : les commandes `edition.*` de ce plan sont des gestes d'édition (editionGestures), jouées par
    // le runner par ObjectEditing et ToolHost, chacune dans sa fenêtre de mesure des redessins du cache. Lu
    // par ce booléen, jamais par le nom.
    bool measuresEdition = false;
    // L5-T31 (S8.5) : > 0, le runner ouvre ce nombre d'onglets de documents synthétiques
    // (tabsFixtureDocument) avant la mesure, relève la mémoire avec un onglet puis avec tous, et joue les
    // commandes `tabs.switch`. Lu par ce nombre, jamais par le nom.
    int tabCount = 0;
    // L5-T31 : un export PDF du document synthétique tourne sur son fil pendant les traits (relancé tant que
    // le scénario dure) ; le runner relève ses durées et la latence d'encre. Lu par ce booléen.
    bool exportsInBackground = false;
    // L5-T31 : le document chargé avant la mesure est tabsFixtureDocument(0) (20 pages de 50 traits).
    bool fixtureDocument = false;
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
// L4-T14 (S7.5, V9). « forme-page-annotee » : page générée portant trois annotations (couche du dessus) ;
// kShapeCount rectangles tracés un à un par glissé avec « tool.shape » (préréglage « rectangle » choisi avant
// la mesure), séparés de kShapePauseNs. Chaque insertion est hors fin de page : une forme va sous les
// annotations et redessine le cache une fois, donc cache.refreshes = k (maxCacheRefreshes = k). Mesure aussi
// le coût de l'aperçu (LiftLayerItem::setObjects reconstruit ses nœuds à chaque mouvement).
inline constexpr int kShapeCount = 10;
inline constexpr std::int64_t kShapePauseNs = 600'000'000; // plus que le redessin du cache en debug (~300 ms)
ScenarioPlan shapeScenario(ScenarioSet set);
// L4-T29 (S7.5, amendements 1 et 5). « miniatures » : les traits de « stroke » joués panneau des navigateurs
// ouvert (« view.browsers » avant la mesure, refermé après), page générée et trois pages de plus
// (kThumbnailPages) pour que le navigateur ait des miniatures à tenir à jour. « miniatures-ferme » : même
// plan, même document, panneau fermé : aucun rendu (L4-T19). Jugés comme « stroke » (figements compris).
inline constexpr int kThumbnailPages = 4;
ScenarioPlan thumbnailsScenario(ScenarioSet set);
ScenarioPlan closedThumbnailsScenario(ScenarioSet set);
// Amendement 1 : « miniatures-pauses » rejoue les traits de « pauses » panneau ouvert, pour que les rendus
// (kSettleMs après la dernière modification) tombent dans les pauses ; ses figements se comparent à ceux de
// « pauses » sans panneau, dans le même JSON. Ne ferme pas sa mesure sur le versement différé (propre à «
// pauses »).
ScenarioPlan thumbnailsPausesScenario(ScenarioSet set);
// Grille d'une page « grille-dessus » : visible, dessinée au-dessus, un niveau (pas de 32), sans accrochage.
core::Grid gridAboveFixture();
// « grille-dessus » : page générée à grille du dessus, traits du scénario « stroke » (différés : aucun
// redessin du cache entre deux versements de la minuterie ; 0 dans la mesure, pauses de 300 ms < délai).
ScenarioPlan gridAboveScenario(ScenarioSet set);
// L4-T28 : « zones-zoom » = le zoom à la molette (100 -> 800 -> 25 -> 100 %) de « zoom » sur une page générée
// réglée à 40 x 53 cadres, le maximum : 91 séparations pointillées à tenir à chaque pas de zoom.
inline constexpr int kBenchZonesAcross = 40;
inline constexpr int kBenchZonesDown = 53;
ScenarioPlan zonesZoomScenario(ScenarioSet scenarioSet);
// L5-T19 (S8.2, D21). « edition » : page générée portant kEditionAnnotations annotations (couche du dessus) ;
// quatre gestes d'édition passés par ObjectEditing et ToolHost, dans l'ordre de editionGestures() : coller
// kEditionPasted objets, les grouper, aligner cinq annotations, mettre une annotation au premier plan. Un
// trait est écrit toutes les kEditionStrokePeriodNs, jamais pendant la fenêtre d'un geste
// (kEditionGestureSpanNs réservées après sa commande). Le runner relève, par geste, les redessins du cache
// terminés (PageView::cacheRefreshCount, jamais cache.refreshes) : clé `edition.refreshes_by_gesture` du
// JSON.
inline constexpr int kEditionAnnotations = 200;
inline constexpr int kEditionPasted = 20;
inline constexpr std::int64_t kEditionStrokePeriodNs = 2'000'000'000;
inline constexpr std::int64_t kEditionGestureSpanNs = 3'500'000'000;
inline constexpr std::size_t kEditionGestureCount = 4;
// Commandes `edition.*` du plan, dans l'ordre joué.
const char* const* editionGestures() noexcept;
ScenarioPlan editionScenario(ScenarioSet scenarioSet);
// L5-T31 (S8.5, D21). « onglets » : kTabsCount documents synthétiques de kTabsPages pages de
// kTabsStrokesPerPage traits, sans image (fixés par le banc), ouverts chacun dans un onglet, puis
// kTabsSwitches bascules (commande `tabs.switch`, argument : l'indice de l'onglet à afficher, jamais deux
// fois le même de suite, en partant de l'onglet 0). « export-fond » : les traits de « stroke » pendant un
// export PDF du document synthétique (exporting::exportPdf). Résultats, jamais une porte.
inline constexpr int kTabsCount = 4;
inline constexpr int kTabsPages = 20;
inline constexpr int kTabsStrokesPerPage = 50;
inline constexpr int kTabsSwitches = 10;
inline constexpr std::int64_t kTabsSwitchPeriodNs = 800'000'000; // complet ; ÷ 5 en court
inline constexpr double kCacheBytesPerPixel = 20.0;              // cache d'une vue (L10-T8)
inline constexpr double kExportInkRatioBudget =
    1.2; // p95 d'encre avec export / p95 de la référence sans export
core::Document tabsFixtureDocument(int index);
// Seuil de `memory_delta_mb` : largeur x hauteur x kCacheBytesPerPixel, en Mio (2^20 o).
double tabsMemoryBudgetMb(const QSize& windowSize) noexcept;
ScenarioPlan tabsScenario(ScenarioSet set);
ScenarioPlan exportBackgroundScenario(ScenarioSet set);
// L5-C12 : « export-fond-reference », jouée juste avant « export-fond » dans le même passage : même page
// (document synthétique), même tracé, aucun export. Sa latence d'encre p95 est le dénominateur du rapport.
ScenarioPlan exportBackgroundReferenceScenario(ScenarioSet set);
// Fenêtres des glissés du plan, de chaque appui à son relâché (décalages depuis le début du scénario).
std::vector<std::pair<std::int64_t, std::int64_t>> dragWindows(const ScenarioPlan& plan);
// stroke, zoom, scroll, page, pauses, gommer, soulever, forme-page-annotee, grille-dessus, miniatures,
// miniatures-ferme, miniatures-pauses, zones-zoom, edition, onglets, export-fond-reference, export-fond
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

// Annotation de départ d'un scénario : un trait de 4 de large, un point par QPointF, à pression moyenne.
core::Stroke seedAnnotation(const std::vector<QPointF>& points);

} // namespace actcraft::bench
