#include <actcraft/bench/verdict.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace actcraft::bench {

namespace {

void addReason(BenchResult& r, const QString& reason)
{
    if (!r.invalidReasons.contains(reason)) {
        r.invalidReasons << reason;
    }
}

// Même condition que le JSON (result.cpp, « invalid_refresh_period »).
bool invalidPeriod(const ScenarioResult& s)
{
    return s.intervals.invalidRefreshPeriod || s.frames.refreshPeriodNs <= 0;
}

bool isTablet(InputKind kind)
{
    return kind != InputKind::Wheel;
}

} // namespace

void applyVerdict(BenchResult& r, const Budget& b)
{
    if (r.machine.buildType != QLatin1String("RelWithDebInfo") &&
        r.machine.buildType != QLatin1String("Release")) {
        addReason(r, QStringLiteral("build-not-release"));
    }
    // Rendu matériel : synchronisation verticale attendue, mémoire graphique lisible (décision 26). Le rendu
    // logiciel est informatif (N1-3) : ni l'une ni l'autre.
    const bool hardware = !r.scenarios.empty() && r.graphicsApi != QLatin1String("software");
    if (hardware) {
        const auto reference = static_cast<double>(r.refresh.referenceNs);
        const auto measured = static_cast<double>(r.refresh.measuredMedianNs);
        if (r.refresh.referenceNs > 0 && (r.refresh.measuredMedianNs <= 0 ||
                                          std::abs(measured - reference) > kRefreshTolerance * reference)) {
            addReason(r, QStringLiteral("invalid-refresh-period")); // l'écran ne présente pas à sa période
        }
    }
    bool periodKnown = true;
    for (const ScenarioResult& s : r.scenarios) {
        if (s.exposureLost) {
            addReason(r, QStringLiteral("exposure-lost"));
        }
        if (invalidPeriod(s)) {
            // Sans période d'écran, classifyIntervals ne calcule rien : juger ces zéros donnerait un faux
            // « BUDGET DEPASSE ».
            addReason(r, QStringLiteral("invalid-refresh-period"));
            periodKnown = false;
        } else if (hardware && s.intervals.active.count > 0 &&
                   s.intervals.active.p50 <
                       kMinActiveP50Periods * static_cast<double>(s.frames.refreshPeriodNs) / 1e6) {
            addReason(r, QStringLiteral("invalid-refresh-period")); // images plus rapides que l'écran
        }
        if (s.latency && (s.latency->unmatched > 0 || s.latency->total.count == 0)) {
            // Échantillons jamais vus dans une image (sonde muette, fin de trait perdue) : le p50 les
            // ignorerait.
            addReason(r, QStringLiteral("latency-unmatched"));
        }
        if (s.releases.notSynced > 0) {
            addReason(r, QStringLiteral("release-not-synced")); // fin de trait rattachée au trait suivant
        }
        if (hardware && !s.memory.gpuPeak) {
            addReason(r, QStringLiteral("gpu-memory-unavailable")); // N1-7 ne serait jugé que sur le privé
        }
    }
    if (!contentCheckOk(r)) {
        addReason(r, QStringLiteral("content-check-failed"));
    }
    r.valid = r.invalidReasons.isEmpty();

    r.criteria.clear();
    const auto criterion = [&r](const QString& scenario, const char* name, double value, double limit,
                                bool ok, bool excluded = false) {
        // Un critère exclu est relevé, jamais dépassé : ok reste vrai (JSON : « excluded »).
        r.criteria.push_back({scenario, QString::fromLatin1(name), value, limit, ok || excluded, excluded});
    };
    std::uint64_t memoryPeak = 0;
    for (const ScenarioResult& s : r.scenarios) {
        const IntervalStats& iv = s.intervals;
        if (!invalidPeriod(s)) {
            criterion(s.name, "fps", iv.fps, b.minFps, iv.fps >= b.minFps);
            criterion(s.name, "p95_ms", iv.active.p95, b.maxP95Ms,
                      iv.active.count > 0 && iv.active.p95 <= b.maxP95Ms);
            criterion(s.name, "over_25ms_ratio", iv.over25Ratio, b.maxOver25Ratio,
                      iv.over25Ratio <= b.maxOver25Ratio);
            // N1-1 et H-orch-1 : figements et reprises, au plus une image perdue chacun. Un scénario de geste
            // (S36, M3) en est exclu par le booléen que son plan pose, jamais par son nom.
            criterion(s.name, "freeze_over_limit", static_cast<double>(iv.freezeOverLimit), 0,
                      iv.freezeOverLimit == 0, s.freezeExempt);
            criterion(s.name, "resume_over_limit", static_cast<double>(iv.resumeOverLimit), 0,
                      iv.resumeOverLimit == 0, s.freezeExempt);
        }
        if (s.maxCacheRefreshes >= 0) {
            // Attendu chiffré S36 : redessins du cache pendant le geste (cache.refreshes du JSON).
            const auto refreshes = static_cast<double>(s.cacheRequestNs.size());
            criterion(s.name, "cache_refreshes", refreshes, s.maxCacheRefreshes,
                      refreshes <= s.maxCacheRefreshes);
        }
        if (s.latency) {
            criterion(s.name, "latency_p50_ms", s.latency->total.p50, b.maxLatencyP50Ms,
                      s.latency->total.count > 0 && s.latency->total.p50 < b.maxLatencyP50Ms);
        }
        memoryPeak = std::max(memoryPeak, s.memory.privatePeak + s.memory.gpuPeak.value_or(0));
    }
    criterion(QStringLiteral("all"), "memory_peak_bytes", static_cast<double>(memoryPeak),
              static_cast<double>(b.maxMemoryBytes), memoryPeak <= b.maxMemoryBytes);
    r.withinBudget =
        !r.scenarios.empty() && periodKnown &&
        std::all_of(r.criteria.begin(), r.criteria.end(), [](const Criterion& c) { return c.ok; });
    r.informative = r.graphicsApi == QLatin1String("software");
}

ExitCode exitCodeFor(const BenchResult& r) noexcept
{
    if (!r.valid) {
        return ExitCode::Invalid;
    }
    return r.withinBudget ? ExitCode::WithinBudget : ExitCode::OverBudget;
}

QString verdictLine(const BenchResult& r)
{
    switch (exitCodeFor(r)) {
    case ExitCode::WithinBudget:
        return QStringLiteral("BUDGET TENU");
    case ExitCode::OverBudget:
        return QStringLiteral("BUDGET DEPASSE");
    case ExitCode::Invalid:
        break;
    }
    return QStringLiteral("MESURE INVALIDE");
}

QStringList summaryLines(const BenchResult& r, const QString& path)
{
    QStringList lines;
    lines << QStringLiteral("Résultat : %1").arg(path);
    lines << QStringLiteral("Rendu : %1 (demandé : %2) · cache : %3 · boucle : %4%5")
                 .arg(r.graphicsApi, r.requestedBackend,
                      r.cache ? QStringLiteral("oui") : QStringLiteral("non"), r.renderLoop,
                      r.informative ? QStringLiteral(" · informatif") : QString());
    lines << QStringLiteral("Période d'écran : %1 ms (%2) · médiane du rendu continu : %3 ms")
                 .arg(static_cast<double>(r.refresh.referenceNs) / 1e6, 0, 'f', 2)
                 .arg(r.refresh.source)
                 .arg(static_cast<double>(r.refresh.measuredMedianNs) / 1e6, 0, 'f', 2);
    for (const ScenarioResult& s : r.scenarios) {
        const IntervalStats& iv = s.intervals;
        QString line = QStringLiteral("%1 : ").arg(s.name);
        if (invalidPeriod(s)) {
            line += QStringLiteral("statistiques non calculées");
        } else {
            line += QStringLiteral("%1 fps · p95 %2 ms · > 25 ms %3 % · figements %4 · reprises %5")
                        .arg(iv.fps, 0, 'f', 1)
                        .arg(iv.active.p95, 0, 'f', 1)
                        .arg(100.0 * iv.over25Ratio, 0, 'f', 2)
                        .arg(iv.freezeOverLimit)
                        .arg(iv.resumeOverLimit);
        }
        if (s.latency) {
            line += QStringLiteral(" · latence p50 %1 ms, p95 %2 ms")
                        .arg(s.latency->total.p50, 0, 'f', 1)
                        .arg(s.latency->total.p95, 0, 'f', 1);
        }
        lines << line;
    }
    for (const Criterion& c : r.criteria) {
        if (!c.ok) {
            lines << QStringLiteral("Dépassé : %1 %2 = %3 (seuil %4)")
                         .arg(c.scenario, c.name)
                         .arg(c.value)
                         .arg(c.limit);
        }
    }
    if (!r.invalidReasons.isEmpty()) {
        lines << QStringLiteral("Invalide : %1").arg(r.invalidReasons.join(QStringLiteral(", ")));
    }
    lines << verdictLine(r);
    return lines;
}

std::int64_t measureRefreshPeriodNs(const std::vector<std::int64_t>& swapNs)
{
    if (swapNs.size() < kMinRefreshIntervals + 1) {
        return 0;
    }
    std::vector<std::int64_t> intervals;
    intervals.reserve(swapNs.size() - 1);
    for (std::size_t i = 1; i < swapNs.size(); ++i) {
        intervals.push_back(swapNs[i] - swapNs[i - 1]);
    }
    // Médiane basse : les images sautées (2 périodes) et la gigue n'y changent rien.
    const auto middle = intervals.begin() + static_cast<std::ptrdiff_t>((intervals.size() - 1) / 2);
    std::nth_element(intervals.begin(), middle, intervals.end());
    return *middle > 0 ? *middle : 0;
}

bool sequenceMatchesPlan(const std::vector<PlannedInput>& inputs, const ReceivedSamples& received)
{
    std::size_t k = 0;
    for (std::size_t i = 0; i < inputs.size(); ++i) {
        if (!isTablet(inputs[i].kind)) {
            continue;
        }
        if (k >= received.size() || received[k].first != i) {
            return false;
        }
        ++k;
    }
    return k == received.size();
}

ReleaseCoverage releaseCoverage(const std::vector<PlannedInput>& inputs, const ReceivedSamples& received,
                                const std::vector<std::int64_t>& syncStartNs,
                                const std::vector<SyncRecord>& records)
{
    std::vector<std::int64_t> receivedAt(inputs.size(), -1);
    for (const auto& [seq, at] : received) {
        if (seq < inputs.size()) {
            receivedAt[seq] = at;
        }
    }
    ReleaseCoverage out;
    for (std::size_t release = 0; release < inputs.size(); ++release) {
        if (inputs[release].kind != InputKind::TabletRelease || receivedAt[release] < 0) {
            continue;
        }
        std::uint64_t nextPress = std::numeric_limits<std::uint64_t>::max();
        std::int64_t until = std::numeric_limits<std::int64_t>::max();
        for (std::size_t j = release + 1; j < inputs.size(); ++j) {
            if (inputs[j].kind == InputKind::TabletPress) {
                nextPress = j;
                if (receivedAt[j] >= 0) {
                    until = receivedAt[j];
                }
                break;
            }
        }
        const auto syncs = std::count_if(syncStartNs.begin(), syncStartNs.end(), [&](std::int64_t t) {
            return t > receivedAt[release] && t < until;
        });
        if (syncs == 0) {
            continue; // relâché et appui suivant reçus d'un bloc : pas de synchronisation entre eux
        }
        ++out.checked;
        const bool covered = std::any_of(records.begin(), records.end(), [&](const SyncRecord& rec) {
            return rec.lastSeq >= release && rec.lastSeq < nextPress;
        });
        if (!covered) {
            ++out.notSynced;
        }
    }
    return out;
}

RefreshPeriod referenceRefreshPeriod(std::int64_t dwmPeriodNs, double screenHz)
{
    RefreshPeriod out;
    if (dwmPeriodNs > 0) {
        out.referenceNs = dwmPeriodNs;
        out.source = QStringLiteral("dwm");
    } else if (screenHz > 0) {
        out.referenceNs = static_cast<std::int64_t>(1e9 / screenHz);
        out.source = QStringLiteral("qscreen");
    }
    return out;
}

} // namespace actcraft::bench
