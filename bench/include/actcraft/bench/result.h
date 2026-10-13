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

struct ScenarioResult
{
    QString name;             // stroke | zoom | scroll
    std::int64_t startNs = 0; // origine des séries brutes
    FrameSeries frames;
    IntervalStats intervals;
    std::optional<LatencyStats> latency; // tracé seulement
    std::vector<double> injectionDelayMs;
    std::vector<double> receptionDelayMs;
    MemorySummary memory;
    bool exposureLost = false;
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
    double refreshHz = 0;
    double devicePixelRatio = 1;
    bool activated = false;
    std::vector<ScenarioResult> scenarios;
    std::vector<ContentCheck> contentChecks;
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
