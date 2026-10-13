#include <actcraft/bench/scenarios.h>

#include <cmath>
#include <numbers>

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

std::vector<ScenarioPlan> allScenarios(ScenarioSet set)
{
    return {strokeScenario(set), zoomScenario(set), scrollScenario(set)};
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

} // namespace actcraft::bench
