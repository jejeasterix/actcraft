#include <actcraft/bench/scenarios.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/generator.h>
#include <actcraft/core/hittest.h>
#include <actcraft/ui/viewtransform.h>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <numbers>
#include <optional>
#include <utility>

namespace actcraft::bench {

core::Stroke seedAnnotation(const std::vector<QPointF>& points)
{
    core::Stroke stroke;
    stroke.baseWidth = 4;
    for (const QPointF& p : points) {
        stroke.points.push_back({p.x(), p.y(), 0.5f});
    }
    return stroke;
}

namespace {
constexpr int kDemoEvents = 120;
constexpr double kDemoY = 670.0;
constexpr double kDemoX0 = 260.0;
constexpr double kDemoLength = 500.0;

constexpr int kSeedCount = 24;

// Annotation posée avant le passage de gomme : trait horizontal ondulé, zone centrale, hors des témoins.
std::vector<QPointF> seedStroke(int index)
{
    std::vector<QPointF> points;
    const double y = 160.0 + 18.0 * index;
    for (double x = 230.0; x <= 790.0; x += 14.0) {
        points.emplace_back(x, y + 3.0 * std::sin(x / 40.0 + index));
    }
    return points;
}

// Page du banc à laquelle s'ajoutent les annotations de départ : seul le décompte des touchées compte ici.
core::Page pageWithSeeds(const std::vector<std::vector<QPointF>>& seeds)
{
    core::Page page = core::generatePage();
    core::ObjectId next = 1;
    for (const core::PageObject& object : page.objects) {
        next = std::max(next, object.common.id + 1);
    }
    for (const auto& points : seeds) {
        core::PageObject object = seedAnnotation(points);
        object.common.layer = core::Layer::Top;
        object.common.id = next++;
        page.objects.push_back(std::move(object));
    }
    return page;
}

// Même critère que EraserTool::sweep : l'emprise de l'annotation touche celle du segment agrandie du rayon.
int touchedAnnotations(const core::Page& page, const std::vector<PlannedInput>& inputs)
{
    std::vector<core::ObjectId> touched;
    for (std::size_t i = 1; i < inputs.size(); ++i) {
        const core::RectF box = core::normalizedRect({inputs[i - 1].pagePos.x(), inputs[i - 1].pagePos.y()},
                                                     {inputs[i].pagePos.x(), inputs[i].pagePos.y()});
        const double r = kEraserRadiusPage;
        const core::RectF reach{box.x - r, box.y - r, box.width + 2 * r, box.height + 2 * r};
        for (const core::ObjectId id : core::objectsTouching(page, reach)) {
            const std::size_t at = core::indexOf(page, id);
            if (at < page.objects.size() && core::isAnnotation(page.objects[at])) {
                touched.push_back(id);
            }
        }
    }
    std::sort(touched.begin(), touched.end());
    touched.erase(std::unique(touched.begin(), touched.end()), touched.end());
    return static_cast<int>(touched.size());
}

// Un trait ondulé (`waves` ondulations) sur la rangée `row`, de l'appui au relâché, un point tous les
// kStylusPeriodNs depuis `start`. Zone centrale (x 220–804, y 150–618) : hors des témoins et de la bande de
// démonstration.
void appendWaveStroke(std::vector<PlannedInput>& inputs, std::int64_t start, int samples, int row, int waves)
{
    for (int i = 0; i < samples; ++i) {
        const double u = static_cast<double>(i) / (samples - 1);
        PlannedInput in;
        in.offsetNs = start + i * kStylusPeriodNs;
        in.kind = i == 0             ? InputKind::TabletPress
                  : i == samples - 1 ? InputKind::TabletRelease
                                     : InputKind::TabletMove;
        in.pagePos = QPointF(220.0 + 584.0 * u,
                             170.0 + 44.0 * row + 20.0 * std::sin(2 * std::numbers::pi * waves * u));
        in.pressure = in.kind == InputKind::TabletRelease
                          ? 0.0f
                          : static_cast<float>(0.5 + 0.4 * std::sin(2 * std::numbers::pi * u));
        in.measured = in.kind != InputKind::TabletRelease;
        inputs.push_back(in);
    }
}

// Ajoute le segment from → to, un point tous les `step` pixels environ ; le premier est l'appui si `first`.
void appendLine(std::vector<PlannedInput>& inputs, QPointF from, QPointF to, double step, std::int64_t& at,
                bool first)
{
    const double length = std::hypot(to.x() - from.x(), to.y() - from.y());
    const int segments = std::max(1, static_cast<int>(std::ceil(length / step)));
    for (int i = first ? 0 : 1; i <= segments; ++i) {
        const double u = static_cast<double>(i) / segments;
        PlannedInput in;
        in.offsetNs = at;
        in.kind = first && i == 0 ? InputKind::TabletPress : InputKind::TabletMove;
        in.pagePos = QPointF(from.x() + u * (to.x() - from.x()), from.y() + u * (to.y() - from.y()));
        in.pressure = 0.5f;
        inputs.push_back(in);
        at += kStylusPeriodNs;
    }
}

void appendRelease(std::vector<PlannedInput>& inputs)
{
    PlannedInput in = inputs.back();
    in.offsetNs += kStylusPeriodNs;
    in.kind = InputKind::TabletRelease;
    in.pressure = 0.0f;
    inputs.push_back(in);
}

struct LiftTarget
{
    QPointF bandFrom;
    QPointF bandTo;
    QPointF grab;
    double dx = 0;
    int taken = 0; // objets pris par le rectangle
};

// Rectangle de sélection tiré depuis la marge de la vue, à gauche de la page (un coin libre : la page générée
// est dense, aucun point de la zone centrale n'est libre), qui prend le moins d'objets possible (deux si la
// page le permet : leurs emprises se recouvrent), hors témoins ; puis un point de l'un d'eux que `objectAt`
// rend, loin des poignées du cadre (huit, aux coins et aux milieux) pour que l'appui saisisse l'objet.
// Parcours déterministe, emprises et points d'appui calculés une fois par objet.
LiftTarget findLiftTarget()
{
    const core::Page page = core::generatePage();
    const std::size_t count = page.objects.size();
    std::vector<core::RectF> bounds;
    bounds.reserve(count);
    for (const core::PageObject& object : page.objects) {
        bounds.push_back(core::boundsOnPage(object));
    }
    // Points d'appui d'un objet : zone centrale, rendus par objectAt, au plus 12 (calculés à la demande).
    std::vector<std::optional<std::vector<QPointF>>> grabs(count);
    const auto grabPoints = [&](std::size_t i) -> const std::vector<QPointF>& {
        if (!grabs[i]) {
            std::vector<QPointF> found;
            const core::ObjectId id = page.objects[i].common.id;
            for (double y = std::max(150.0, bounds[i].y);
                 y <= std::min(618.0, bounds[i].y + bounds[i].height); y += 6) {
                for (double x = std::max(220.0, bounds[i].x);
                     x <= std::min(804.0, bounds[i].x + bounds[i].width); x += 6) {
                    const auto hit = core::objectAt(page, core::PointF{x, y}, core::kHitToleranceScreen);
                    if (hit && *hit == id && (x + 300.0 <= 804.0 || x - 300.0 >= 220.0)) {
                        found.emplace_back(x, y);
                    }
                }
            }
            grabs[i] = std::move(found);
        }
        return *grabs[i];
    };
    constexpr double x0 = -30;
    for (std::size_t want = 2; want <= 8; ++want) {
        for (const double height : {6.0, 12.0}) {
            for (double y0 = 150; y0 + height <= 618; y0 += 6) {
                for (double width = 36; width <= 420; width += 6) {
                    const core::RectF rect{x0, y0, width, height};
                    std::vector<std::size_t> taken;
                    for (std::size_t i = 0; i < count && taken.size() <= want; ++i) {
                        if (core::rectsTouch(bounds[i], rect)) {
                            taken.push_back(i);
                        }
                    }
                    if (taken.size() != want ||
                        taken.back() >= count - 3) { // hors témoins (les trois derniers)
                        continue;
                    }
                    core::RectF frame = bounds[taken[0]];
                    for (const std::size_t i : taken) {
                        frame = core::unitedRect(frame, bounds[i]);
                    }
                    const double hx[] = {frame.x, frame.x + frame.width / 2, frame.x + frame.width};
                    const double hy[] = {frame.y, frame.y + frame.height / 2, frame.y + frame.height};
                    for (const std::size_t i : taken) {
                        for (const QPointF& p : grabPoints(i)) {
                            double nearest = 1e9;
                            for (const double px : hx) {
                                for (const double py : hy) {
                                    nearest = std::min(nearest, std::hypot(p.x() - px, p.y() - py));
                                }
                            }
                            if (nearest >= 12.0) {
                                LiftTarget target;
                                target.bandFrom = QPointF(x0, y0);
                                target.bandTo = QPointF(x0 + width, y0 + height);
                                target.grab = p;
                                target.dx = p.x() + 300.0 <= 804.0 ? 300.0 : -300.0;
                                target.taken = static_cast<int>(want);
                                return target;
                            }
                        }
                    }
                }
            }
        }
    }
    return {};
}
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
        appendWaveStroke(plan.inputs, start, samples, k, 3);
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
    plan.pageChecks = true;  // relevés du zoom 150 et contrôle de l'éclair (runner.cpp)
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

ScenarioPlan pausesScenario(ScenarioSet set)
{
    const bool court = set == ScenarioSet::Court;
    const int strokes = court ? 4 : 10;
    const int samples = court ? 40 : 100; // 0,2 s ou 0,5 s à 200 points/s
    const std::int64_t idleNs = std::int64_t{ui::kCacheFoldIdleMs} * 1'000'000;
    ScenarioPlan plan;
    plan.name = QStringLiteral("pauses");
    plan.zoom = 1.0;
    plan.generatedPages = 1;
    plan.awaitFinalFold = true; // au moins un versement mesuré, quelle que soit la charge (L3-C14)
    std::int64_t start = 0;
    for (int k = 0; k < strokes; ++k) {
        appendWaveStroke(plan.inputs, start, samples, k, 2);
        const int step = court ? 3 * k : k;
        start += (samples - 1) * kStylusPeriodNs + idleNs + step * kPausesStepNs;
    }
    return plan;
}

ScenarioPlan eraseScenario(ScenarioSet set)
{
    const int sweeps = std::max(2, 8 / durationDivisor(set)); // diagonales du zigzag
    ScenarioPlan plan;
    plan.name = QStringLiteral("gommer");
    plan.zoom = 1.0;
    plan.generatedPages = 1;
    plan.toolCommand = QStringLiteral("tool.eraser");
    for (int k = 0; k < kSeedCount; ++k) {
        plan.seedStrokes.push_back(seedStroke(k));
    }
    // Zigzag sur la zone des annotations (x 240–780, y 140–600) : chaque diagonale les croise toutes.
    std::int64_t at = 0;
    const double dx = 540.0 / sweeps;
    for (int i = 0; i < sweeps; ++i) {
        const QPointF from(240.0 + dx * i, i % 2 == 0 ? 140.0 : 600.0);
        const QPointF to(240.0 + dx * (i + 1), i % 2 == 0 ? 600.0 : 140.0);
        appendLine(plan.inputs, from, to, 8.0, at, i == 0);
    }
    appendRelease(plan.inputs);
    plan.touchedStrokes = touchedAnnotations(pageWithSeeds(plan.seedStrokes), plan.inputs);
    plan.pendingCut = 0;      // tout est versé avant la mesure : aucun trait n'attend au-dessus du cache
    plan.freezeExempt = true; // S36 : hors du critère « 0 figement » de S13
    plan.maxCacheRefreshes = plan.touchedStrokes + 1 + plan.pendingCut;
    return plan;
}

ScenarioPlan liftScenario(ScenarioSet set)
{
    static const LiftTarget target = findLiftTarget(); // déterministe : calculé une fois
    ScenarioPlan plan;
    plan.name = QStringLiteral("soulever");
    plan.zoom = 1.0;
    plan.generatedPages = 1;
    plan.toolCommand = QStringLiteral("tool.select");
    plan.liftedObjects = target.taken;
    plan.freezeExempt = true;   // S36 : hors du critère « 0 figement » de S13
    plan.maxCacheRefreshes = 2; // S36 : deux redessins (pose de la sélection, relâché)
    std::int64_t at = 0;
    appendLine(plan.inputs, target.bandFrom, target.bandTo, 4.0, at, true);
    appendRelease(plan.inputs);
    at = plan.inputs.back().offsetNs + 300'000'000 / durationDivisor(set); // la sélection se pose
    const int samples = static_cast<int>(2'000'000'000LL / durationDivisor(set) / kStylusPeriodNs);
    for (int i = 0; i <= samples; ++i) {
        PlannedInput in;
        in.offsetNs = at + i * kStylusPeriodNs;
        in.kind = i == 0 ? InputKind::TabletPress : InputKind::TabletMove;
        in.pagePos = QPointF(target.grab.x() + target.dx * i / samples, target.grab.y());
        in.pressure = 0.5f;
        plan.inputs.push_back(in);
    }
    appendRelease(plan.inputs);
    return plan;
}

std::vector<ScenarioPlan> allScenarios(ScenarioSet set)
{
    return {strokeScenario(set), zoomScenario(set),  scrollScenario(set), pageScenario(set),
            pausesScenario(set), eraseScenario(set), liftScenario(set)};
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
