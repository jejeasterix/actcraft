#pragma once

#include <actcraft/bench/machineinfo.h>
#include <actcraft/bench/memory.h>
#include <actcraft/bench/stats.h>

#include <QDateTime>
#include <QJsonObject>
#include <QSGRendererInterface>
#include <QSize>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace actcraft::bench {

inline constexpr char kResultFormat[] = "actcraft-bench/1";

struct ContentCheck
{
    QString name;
    bool ok = false;
    QString expected;
    QString actual;
};

struct Criterion
{
    QString scenario;
    QString name;
    double value = 0;
    double limit = 0;
    bool ok = false;
    // Critère retiré du jugement pour ce scénario (ScenarioResult::freezeExempt) : relevé, jamais dépassé.
    bool excluded = false;
};

// Contrôle du rattachement des traits à leurs images (verdict.h, releaseCoverage).
struct ReleaseCoverage
{
    std::size_t checked = 0;   // relâchés suivis d'une synchronisation avant l'appui suivant
    std::size_t notSynced = 0; // parmi eux, relâchés qu'aucune synchronisation de leur trait ne couvre
};

// Période d'écran : la référence fixe pause et limites (classifyIntervals) ; la médiane mesurée la contrôle.
struct RefreshPeriod
{
    std::int64_t referenceNs = 0;            // période de l'écran ; ≤ 0 : inconnue (mesure invalide)
    QString source = QStringLiteral("none"); // dwm | qscreen | none
    std::int64_t measuredMedianNs =
        0; // médiane des intervalles d'un rendu continu (chauffe) ; 0 : non mesurée
};

// L4-T14 (amendement 4, V9) : trames pendant les glissés de forme (clé shape_preview) et leurs intervalles.
// Un résultat, jamais une porte (hors budget, H4-8).
struct ShapePreviewStats
{
    int frames = 0;   // images présentées pendant un glissé
    double p95Ms = 0; // p95 des intervalles qui précèdent ces images (0 si aucun)
    double maxMs = 0;
};

// `swapNs` : instants des images (horloge du banc) ; `windowsNs` : fenêtres [début, fin] des glissés, mêmes
// unités. Une image compte si son instant tombe dans une fenêtre ; son intervalle est celui qui la sépare de
// l'image précédente (aucun pour la toute première).
ShapePreviewStats shapePreviewStats(const std::vector<std::int64_t>& swapNs,
                                    const std::vector<std::pair<std::int64_t, std::int64_t>>& windowsNs);

// L4-T29 (S7.5, amendements 1 et 5) : miniatures des pages pendant la fenêtre mesurée (clé thumbnails).
// Un résultat, jamais une porte.
struct ThumbnailStats
{
    int renders = 0;       // rendus terminés entre le début et la fin de la mesure (renderCount)
    int duringContact = 0; // rendus lancés (launchCount) entre un appui et son relâché
    double maxCopyMs = 0;  // copie de la page la plus longue (lastCopyMs relevé à chaque lancement)
};

// Rendus lancés (instants `launchNs`, horloge du banc) qui tombent dans un contact : `windowsNs` sont les
// intervalles appui -> relâché injectés, bornes comprises. Un lancement par rendu (L4-T29, N10).
int launchesDuringContact(const std::vector<std::int64_t>& launchNs,
                          const std::vector<std::pair<std::int64_t, std::int64_t>>& windowsNs);

// L5-T19 (D21) : redessins du cache terminés (PageView::cacheRefreshCount) pendant la fenêtre de chaque geste
// du scénario « edition » ; -1 : geste non mesuré. Un résultat, jamais une porte (attendu : chacun <= 1).
struct EditionRefreshes
{
    int paste = -1;
    int group = -1;
    int align = -1;
    int front = -1;
};

// L5-T31 (S8.5) : clé `tabs` de « onglets ». memoryDeltaMb = mémoire privée avec kTabsCount onglets moins
// celle avec un seul (même document synthétique dans l'onglet 1), en Mio (2^20 o) ; seuil = largeur x hauteur
// x 20 o. Un résultat, jamais une porte.
struct TabsStats
{
    double memoryOneTabMb = 0;
    double memoryFourTabsMb = 0;
    double memoryDeltaMb = 0;
    double memoryBudgetMb = 0;
    std::vector<double> switchMs; // de la commande à la page affichée et son cache redessiné
};

// L5-T31 : clé `export_background` de « export-fond ». Latence d'encre p95 pendant les exports PDF contre
// celle de la référence sans export, même page et même tracé, du même passage (budget 1,2) ; « stroke »
// (page de 300 objets) n'est gardé que pour information. Un résultat, jamais une porte.
struct ExportBackgroundStats
{
    int exports = 0;  // exports terminés pendant la mesure
    int failures = 0; // dont en échec
    double maxExportMs = 0;
    double inkP95Ms = 0;
    double strokeInkP95Ms = 0;
    double baselineInkP95Ms = 0; // L5-C12 : « export-fond-reference »
    // L5-C14 : tours du fil de charge témoin de la référence (SpinningLoad) ; 0 : la référence n'a porté
    // aucune charge (mesure à ne pas comparer à celles de L5-C14).
    std::int64_t baselineControlLoadIterations = 0;
    int freezes = 0;
    int baselineFreezes = 0;
    int strokeFreezes = 0;
};

struct ScenarioResult
{
    QString name;             // stroke | zoom | scroll | page | pauses | gommer | soulever
    std::int64_t startNs = 0; // origine des séries brutes
    FrameSeries frames;
    IntervalStats intervals;
    std::optional<LatencyStats> latency; // tracé seulement
    std::vector<double> injectionDelayMs;
    std::vector<double> receptionDelayMs;
    MemorySummary memory;
    bool exposureLost = false;
    ReleaseCoverage releases; // tracé seulement
    // L10-T1 : PageView::cacheRefreshRequested et cacheUpdated pendant le scénario (horloge
    // core::monotonicNowNs()), pour attribuer un figement ou une reprise à un redessin du cache.
    std::vector<std::int64_t> cacheRequestNs;
    std::vector<std::int64_t> cacheUpdatedNs;
    int pendingMax = 0; // L10-T4 : plus grand FixedPageItem::pendingCount pendant le scénario
    // L3-T19 : relevés de « gommer » (k, 1 si le dernier trait en attente est gommé en deux morceaux) ; -1 :
    // sans objet. Reconstructions complètes et décodages d'images de la page fixe pendant le scénario (un
    // soulèvement qui redécode les images des synthétiques s'y voit) ; -1 : non relevés.
    int touchedStrokes = -1;
    int pendingCut = -1;
    int liftedObjects = -1; // « soulever » : objets pris par le rectangle de sélection
    // L3-C10 (S36, M3) : posé par le plan du scénario, jamais déduit de son nom. true : figements et reprises
    // hors limite ne sont pas jugés (relevés, marqués « excluded » dans le verdict) ; fps, p95 et part des
    // intervalles > 25 ms le restent.
    bool freezeExempt = false;
    std::int64_t controlLoadIterations = 0; // L5-C14 : tours de la charge témoin (plan.spinsInBackground)
    // Attendu chiffré S36 sur cache.refreshes (cacheRequestNs.size()) ; < 0 : pas de critère.
    int maxCacheRefreshes = -1;
    int rebuilds = -1;
    int decodes = -1;
    // L4-T14 : k formes posées (cache.placed_shapes ; -1 : sans objet) et trames pendant les glissés
    // (shape_preview ; absent : scénario qui ne trace pas de forme).
    int placedShapes = -1;
    std::optional<ShapePreviewStats> shapePreview;
    std::optional<EditionRefreshes> edition;               // L5-T19 : absent hors « edition »
    std::optional<TabsStats> tabs;                         // L5-T31 : absent hors « onglets »
    std::optional<ExportBackgroundStats> exportBackground; // L5-T31 : absent hors « export-fond »
    std::optional<ThumbnailStats>
        thumbnails; // L4-T29 : absent hors « miniatures », « miniatures-ferme » et « miniatures-pauses »
};

struct BenchResult
{
    QDateTime startedAt;
    MachineInfo machine;
    QString graphicsApi = QStringLiteral("unknown");
    QString requestedBackend;
    QString renderLoop;
    QString gpuName;
    bool cache = true;
    QString scenarioSet;
    QString windowMode;
    QSize windowSize;
    QString screenName;
    QSize screenSize;
    double refreshHz = 0; // nominale (QScreen)
    RefreshPeriod refresh;
    double devicePixelRatio = 1;
    bool activated = false;
    std::vector<ScenarioResult> scenarios;
    std::vector<ContentCheck> contentChecks;
    // Scénario « page » (L2-T23) ; -1 s'il n'a pas été joué. pageFlash : captures blanches au témoin de forme
    // juste après un changement de page (contentCheckOk faux si > 0). Entre le zoom posé (view.zoom) et la
    // fin du scénario, sur la page peinte : reconstructions complètes de la page fixe (rebuildCount) et nœuds
    // rastérisés refaits (rasterizationCount ; 0 en rendu logiciel, nœuds vectoriels).
    int pageFlash = -1;       // captures SANS cache seulement (celles qui discriminent)
    int pageFlashCached = -1; // relevé : captures avec cache (la vue masque la page fixe jusqu'au prêt)
    int pageFlashProbes = -1; // captures sans cache retenues (nouvelle page pas encore prête à la capture)
    int pageRebuildsDuringZoom = -1;
    int pageRasterizationsDuringZoom = -1;
    std::vector<Criterion> criteria;
    QStringList invalidReasons;
    bool valid = false;
    bool withinBudget = false;
    bool informative = false;
};

QString apiName(QSGRendererInterface::GraphicsApi api);
// L6-C11 : une erreur du parcours (« error: … ») ; seule elle saute la démonstration et ses contrôles de
// contenu. Une raison d'invalidité relevée avant (page-flash-unverified) ne les supprime pas.
bool hasRunError(const BenchResult& result);
// Au moins un contrôle, et tous réussis.
bool contentCheckOk(const BenchResult& result);
QJsonObject toJson(const BenchResult& result);
// <AAAA-MM-JJ_HHMMSS>-<modèle>-<rendu>.json
QString resultFileName(const BenchResult& result);
// Crée les dossiers manquants ; écriture atomique.
bool writeResult(const BenchResult& result, const QString& path, QString& error);

} // namespace actcraft::bench
