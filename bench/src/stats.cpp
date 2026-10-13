#include <actcraft/bench/stats.h>

#include <algorithm>
#include <cmath>

namespace actcraft::bench {

namespace {
double toMs(std::int64_t ns)
{
    return static_cast<double>(ns) / 1'000'000.0;
}
} // namespace

double percentile(std::vector<double> values, double p)
{
    if (values.empty()) {
        return 0;
    }
    std::sort(values.begin(), values.end());
    // p × n d'abord : exact pour p entier, alors que p / 100.0 arrondi fausse le rang (p = 7, n = 100 → 8).
    const double rank = std::ceil(p * static_cast<double>(values.size()) / 100.0);
    const std::size_t index = rank < 1 ? 0 : std::min(values.size() - 1, static_cast<std::size_t>(rank) - 1);
    return values[index];
}

Percentiles summarize(const std::vector<double>& values)
{
    Percentiles r;
    r.count = values.size();
    if (values.empty()) {
        return r;
    }
    r.p50 = percentile(values, 50);
    r.p95 = percentile(values, 95);
    r.p99 = percentile(values, 99);
    r.max = *std::max_element(values.begin(), values.end());
    return r;
}

IntervalStats classifyIntervals(const FrameSeries& s)
{
    IntervalStats r;
    if (s.refreshPeriodNs <= 0) {
        r.invalidRefreshPeriod = true;
        return r;
    }
    const std::int64_t pauseNs = 2 * s.refreshPeriodNs;
    const auto inputs = s.inputNs.begin();
    const auto inputsEnd = s.inputNs.end();
    for (std::size_t i = 1; i < s.swapNs.size(); ++i) {
        const std::int64_t start = s.swapNs[i - 1];
        const std::int64_t end = s.swapNs[i];
        // Intervalle (start, end] : upper_bound écarte l'instant de l'image de départ, `<= end` garde celui
        // de l'image d'arrivée.
        const auto first = std::upper_bound(inputs, inputsEnd, start);
        const bool hasInput = first != inputsEnd && *first <= end;
        const bool pauseBefore = hasInput && *first - start > pauseNs;
        const auto freeze = std::upper_bound(s.freezeNs.begin(), s.freezeNs.end(), start);
        if (freeze != s.freezeNs.end() && *freeze <= end) {
            const auto next = std::upper_bound(inputs, inputsEnd, *freeze);
            if (next != inputsEnd && *next <= end && *next - *freeze > pauseNs) {
                // Pause après le figement sans image entre les deux : sa durée n'est pas mesurable, et
                // l'image d'arrivée est celle de la reprise.
                ++r.freezeUnmeasured;
                r.classes.push_back(IntervalClass::Resume);
                r.resumeMs.push_back(toMs(end - *next));
                continue;
            }
            // Une pause qui précède la première entrée (posée avant le figement) n'en fait pas partie.
            const std::int64_t origin = pauseBefore && *first <= *freeze ? *first : start;
            r.classes.push_back(IntervalClass::Freeze);
            r.freezeMs.push_back(toMs(end - origin));
            continue;
        }
        if (!hasInput) {
            r.classes.push_back(IntervalClass::Idle);
        } else if (pauseBefore) {
            r.classes.push_back(IntervalClass::Resume);
            r.resumeMs.push_back(toMs(end - *first));
        } else {
            r.classes.push_back(IntervalClass::Active);
            r.activeMs.push_back(toMs(end - start));
        }
    }
    r.active = summarize(r.activeMs);
    if (!r.activeMs.empty()) {
        r.fps = r.active.p50 > 0 ? 1000.0 / r.active.p50 : 0;
        const auto over =
            std::count_if(r.activeMs.begin(), r.activeMs.end(), [](double v) { return v > kOver25Ms; });
        r.over25Ratio = static_cast<double>(over) / static_cast<double>(r.activeMs.size());
    }
    const double limitMs = kFreezeFactor * toMs(s.refreshPeriodNs);
    const auto overLimit = [limitMs](const std::vector<double>& values, double& maxMs, std::size_t& count) {
        for (double v : values) {
            maxMs = std::max(maxMs, v);
            if (v >= limitMs) {
                ++count;
            }
        }
    };
    overLimit(r.freezeMs, r.freezeMaxMs, r.freezeOverLimit);
    overLimit(r.resumeMs, r.resumeMaxMs, r.resumeOverLimit);
    return r;
}

LatencyStats computeLatency(const std::vector<std::int64_t>& plannedNs,
                            const std::vector<SyncRecord>& records)
{
    LatencyStats r;
    std::size_t k = 0;
    for (std::size_t seq = 0; seq < plannedNs.size(); ++seq) {
        if (plannedNs[seq] < 0) {
            continue;
        }
        while (k < records.size() && records[k].lastSeq < seq) {
            ++k;
        }
        if (k == records.size()) {
            ++r.unmatched;
            continue;
        }
        const SyncRecord& rec = records[k];
        r.seq.push_back(static_cast<std::uint64_t>(seq));
        r.totalMs.push_back(toMs(rec.swapNs - plannedNs[seq]));
        r.plannedToSyncMs.push_back(toMs(rec.syncNs - plannedNs[seq]));
        r.syncToSwapMs.push_back(toMs(rec.swapNs - rec.syncNs));
    }
    r.total = summarize(r.totalMs);
    r.plannedToSync = summarize(r.plannedToSyncMs);
    r.syncToSwap = summarize(r.syncToSwapMs);
    return r;
}

} // namespace actcraft::bench
