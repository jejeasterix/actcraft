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

#include <optional>
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
    int rebuilds = -1;
    int decodes = -1;
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
// Au moins un contrôle, et tous réussis.
bool contentCheckOk(const BenchResult& result);
QJsonObject toJson(const BenchResult& result);
// <AAAA-MM-JJ_HHMMSS>-<modèle>-<rendu>.json
QString resultFileName(const BenchResult& result);
// Crée les dossiers manquants ; écriture atomique.
bool writeResult(const BenchResult& result, const QString& path, QString& error);

} // namespace actcraft::bench
