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
    for (std::size_t i = 1; i < s.swapNs.size(); ++i) {
        const std::int64_t start = s.swapNs[i - 1];
        const std::int64_t end = s.swapNs[i];
        const double length = toMs(end - start);
        const auto freeze = std::upper_bound(s.freezeNs.begin(), s.freezeNs.end(), start);
        if (freeze != s.freezeNs.end() && *freeze <= end) {
            r.classes.push_back(IntervalClass::Freeze);
            r.freezeMs.push_back(length);
            continue;
        }
        const auto input = std::upper_bound(s.inputNs.begin(), s.inputNs.end(), start);
        if (input == s.inputNs.end() || *input > end || *input - start > 2 * s.refreshPeriodNs) {
            r.classes.push_back(IntervalClass::Idle);
            continue;
        }
        r.classes.push_back(IntervalClass::Active);
        r.activeMs.push_back(length);
    }
    r.active = summarize(r.activeMs);
    if (!r.activeMs.empty()) {
        r.fps = r.active.p50 > 0 ? 1000.0 / r.active.p50 : 0;
        const auto over =
            std::count_if(r.activeMs.begin(), r.activeMs.end(), [](double v) { return v > kOver25Ms; });
        r.over25Ratio = static_cast<double>(over) / static_cast<double>(r.activeMs.size());
    }
    const double limitMs = kFreezeFactor * toMs(s.refreshPeriodNs);
    for (double v : r.freezeMs) {
        r.freezeMaxMs = std::max(r.freezeMaxMs, v);
        if (v >= limitMs) {
            ++r.freezeOverLimit;
        }
    }
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
