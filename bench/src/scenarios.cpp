#include <actcraft/bench/scenarios.h>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <numbers>
#include <utility>

namespace actcraft::bench {

namespace {
constexpr int kDemoEvents = 120;
constexpr double kDemoY = 670.0;
constexpr double kDemoX0 = 260.0;
constexpr double kDemoLength = 500.0;
} // namespace

int durationDivisor(ScenarioSet set) noexcept
{
    return set == ScenarioSet::Court ? 5 : 1;
}

ScenarioPlan strokeScenario(ScenarioSet set)
{
    const int d = durationDivisor(set);
    const int samples = 300 / d;                  // 1,5 s à 200 points/s
    const std::int64_t pauseNs = 300'000'000 / d; // stylo levé entre deux traits
    ScenarioPlan plan;
    plan.name = QStringLiteral("stroke");
    plan.zoom = 1.0;
    std::int64_t start = 0;
    for (int k = 0; k < 10; ++k) {
        for (int i = 0; i < samples; ++i) {
            const double u = static_cast<double>(i) / (samples - 1);
            PlannedInput in;
            in.offsetNs = start + i * kStylusPeriodNs;
            in.kind = i == 0             ? InputKind::TabletPress
                      : i == samples - 1 ? InputKind::TabletRelease
                                         : InputKind::TabletMove;
            // Zone centrale (x 220–804, y 150–618) : hors des témoins et de la bande de démonstration.
            in.pagePos =
                QPointF(220.0 + 584.0 * u, 170.0 + 44.0 * k + 20.0 * std::sin(2 * std::numbers::pi * 3 * u));
            in.pressure = in.kind == InputKind::TabletRelease
                              ? 0.0f
                              : static_cast<float>(0.5 + 0.4 * std::sin(2 * std::numbers::pi * u));
            in.measured = in.kind != InputKind::TabletRelease;
            plan.inputs.push_back(in);
        }
        start += (samples - 1) * kStylusPeriodNs + pauseNs;
    }
    return plan;
}

std::vector<int> cumulativeSplit(int total, int count)
{
    std::vector<int> out;
    if (count <= 0) {
        return out;
    }
    out.reserve(static_cast<std::size_t>(count));
    long long previous = 0;
    for (int i = 1; i <= count; ++i) {
        const long long current = std::llround(static_cast<double>(total) * i / count);
        out.push_back(static_cast<int>(current - previous));
        previous = current;
    }
    return out;
}

ScenarioPlan zoomScenario(ScenarioSet set)
{
    const int totalEvents = 720 / durationDivisor(set); // 6 s à 120 Hz
    const double ratios[] = {8.0, 1.0 / 32.0, 4.0};     // 100 → 800 → 25 → 100 %
    const double totalLog = std::log(1024.0);
    ScenarioPlan plan;
    plan.name = QStringLiteral("zoom");
    plan.zoom = 1.0;
    std::int64_t index = 0;
    for (double ratio : ratios) {
        const int events = static_cast<int>(std::lround(totalEvents * std::abs(std::log(ratio)) / totalLog));
        // Facteur de la vue : 1,15^(delta/120) (ui::wheelZoomFactor, N2-11).
        const int totalDelta = static_cast<int>(std::lround(120.0 * std::log(ratio) / std::log(1.15)));
        for (int delta : cumulativeSplit(totalDelta, events)) {
            PlannedInput in;
            in.offsetNs = index * kWheelPeriodNs;
            in.kind = InputKind::Wheel;
            in.angleDelta = QPoint(0, delta);
            in.modifiers = Qt::ControlModifier;
            plan.inputs.push_back(in);
            ++index;
        }
    }
    return plan;
}

ScenarioPlan scrollScenario(ScenarioSet set)
{
    const int perLeg = 180 / durationDivisor(set); // 1,5 s à 120 Hz par aller ou retour
    // 20 / 120 cran × 60 px par cran = 10 px par événement (ui::wheelScrollDelta).
    const QPoint legs[] = {QPoint(0, -20), QPoint(0, 20), QPoint(-20, 0), QPoint(20, 0)};
    ScenarioPlan plan;
    plan.name = QStringLiteral("scroll");
    plan.zoom = 4.0; // à 200 % en 1920 px, l'horizontal ne parcourrait qu'environ 128 px
    plan.topLeft = true;
    std::int64_t index = 0;
    for (const QPoint& leg : legs) {
        for (int i = 0; i < perLeg; ++i) {
            PlannedInput in;
            in.offsetNs = index * kWheelPeriodNs;
            in.kind = InputKind::Wheel;
            in.angleDelta = leg;
            plan.inputs.push_back(in);
            ++index;
        }
    }
    return plan;
}

ScenarioPlan pageScenario(ScenarioSet set)
{
    constexpr std::int64_t kFlipPeriodNs = 500'000'000;
    constexpr std::int64_t kZoomPeriodNs = 700'000'000;
    ScenarioPlan plan;
    plan.name = QStringLiteral("page");
    plan.generatedPages = 2; // page 1 : images ; page 2 : images, textes et formes à chemins (moteur peint)
    const int flips = 20 / durationDivisor(set); // nombre pair : la série finit sur la page 1
    for (int i = 0; i < flips; ++i) {
        PlannedInput in;
        in.offsetNs = static_cast<std::int64_t>(i) * kFlipPeriodNs;
        in.kind = InputKind::Command;
        in.command = i % 2 == 0 ? QStringLiteral("page.next") : QStringLiteral("page.previous");
        plan.inputs.push_back(in);
    }
    // Sur la page peinte, deux zooms posés : rastérisation à la nouvelle échelle en RHI (L2-T13).
    const std::int64_t end = static_cast<std::int64_t>(flips) * kFlipPeriodNs;
    const std::pair<QString, QVariant> zooms[] = {{QStringLiteral("page.next"), QVariant()},
                                                  {QStringLiteral("view.zoom"), QVariant(150)},
                                                  {QStringLiteral("view.fit"), QVariant()}};
    for (std::size_t k = 0; k < std::size(zooms); ++k) {
        PlannedInput in;
        in.offsetNs = end + static_cast<std::int64_t>(k) * kZoomPeriodNs;
        in.kind = InputKind::Command;
        in.command = zooms[k].first;
        in.argument = zooms[k].second;
        plan.inputs.push_back(in);
    }
    return plan;
}

std::vector<ScenarioPlan> allScenarios(ScenarioSet set)
{
    return {strokeScenario(set), zoomScenario(set), scrollScenario(set), pageScenario(set)};
}

std::vector<PlannedInput> demoStroke()
{
    std::vector<PlannedInput> out;
    for (int i = 0; i < kDemoEvents; ++i) {
        PlannedInput in;
        in.offsetNs = i * kStylusPeriodNs;
        in.kind = i == 0                 ? InputKind::TabletPress
                  : i == kDemoEvents - 1 ? InputKind::TabletRelease
                                         : InputKind::TabletMove;
        in.pagePos = QPointF(kDemoX0 + kDemoLength * i / (kDemoEvents - 1), kDemoY);
        in.pressure = in.kind == InputKind::TabletRelease ? 0.0f : 0.6f;
        in.measured = false;
        out.push_back(in);
    }
    return out;
}

QPointF demoInkProbePoint()
{
    return QPointF(kDemoX0 + kDemoLength * 30 / (kDemoEvents - 1), kDemoY);
}

std::int64_t planDurationNs(const ScenarioPlan& plan)
{
    return plan.inputs.empty() ? 0 : plan.inputs.back().offsetNs;
}

std::vector<std::int64_t> pageChangeMarks(const std::vector<std::int64_t>& swapNs,
                                          const std::vector<std::pair<std::int64_t, std::int64_t>>& changes)
{
    std::vector<std::int64_t> marks;
    for (const auto& [command, shown] : changes) {
        const auto after =
            std::upper_bound(swapNs.begin(), swapNs.end(), shown); // première image après le prêt
        const std::int64_t until = after == swapNs.end() ? std::numeric_limits<std::int64_t>::max() : *after;
        for (std::size_t k = 1; k < swapNs.size(); ++k) {
            if (swapNs[k] >= command && swapNs[k - 1] < until) {
                marks.push_back(swapNs[k - 1] + 1);
            }
        }
    }
    std::sort(marks.begin(), marks.end());
    marks.erase(std::unique(marks.begin(), marks.end()), marks.end());
    return marks;
}

std::vector<std::pair<std::int64_t, std::int64_t>>
pageChangeWindows(const std::vector<std::int64_t>& commandNs, const std::vector<bool>& awaitsReady,
                  const std::vector<std::int64_t>& shownNs)
{
    std::vector<std::pair<std::int64_t, std::int64_t>> windows;
    for (std::size_t i = 0; i < commandNs.size(); ++i) {
        const std::int64_t at = commandNs[i];
        if (at < 0) {
            continue; // commande jamais jouée
        }
        if (i < awaitsReady.size() && !awaitsReady[i]) {
            windows.emplace_back(at, at);
            continue;
        }
        const auto shown = std::lower_bound(shownNs.begin(), shownNs.end(), at);
        windows.emplace_back(at, shown == shownNs.end() ? std::numeric_limits<std::int64_t>::max() : *shown);
    }
    return windows;
}

} // namespace actcraft::bench
