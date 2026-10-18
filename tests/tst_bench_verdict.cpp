#include "startup.h"

#include <actcraft/bench/injector.h>
#include <actcraft/bench/recorder.h>
#include <actcraft/bench/runner.h>
#include <actcraft/bench/scenarios.h>
#include <actcraft/bench/verdict.h>
#include <actcraft/core/clock.h>
#include <actcraft/core/generator.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/pageview.h>

#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <algorithm>
#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;
using namespace actcraft::bench;

namespace {

constexpr std::uint64_t kMo = 1024ull * 1024ull;
constexpr std::int64_t kPeriod60Ns = 16'666'667;

BenchResult good()
{
    BenchResult r;
    r.machine.buildType = QStringLiteral("RelWithDebInfo");
    r.graphicsApi = QStringLiteral("direct3d11");
    r.contentChecks = {
        {QStringLiteral("witness-shape"), true, QStringLiteral("#ff1e88e5"), QStringLiteral("#ff1e88e5")}};
    r.refresh = {kPeriod60Ns, QStringLiteral("dwm"), kPeriod60Ns + 100'000};
    ScenarioResult s;
    s.name = QStringLiteral("stroke");
    s.frames.refreshPeriodNs = kPeriod60Ns;
    s.intervals.fps = 60;
    s.intervals.active.p50 = 16.6;
    s.intervals.active.p95 = 17;
    s.intervals.active.count = 100;
    s.intervals.over25Ratio = 0;
    s.intervals.freezeOverLimit = 0;
    s.intervals.resumeOverLimit = 0;
    LatencyStats l;
    l.total.p50 = 15;
    l.total.count = 100;
    s.latency = l;
    s.memory.privatePeak = 300 * kMo;
    s.memory.gpuPeak = 200 * kMo;
    r.scenarios.push_back(s);
    return r;
}

ExitCode after(BenchResult r)
{
    applyVerdict(r);
    return exitCodeFor(r);
}

// Traits de `samples` événements (appui, déplacements, relâché), séparés par `pauseNs`.
std::vector<PlannedInput> strokes(int count, int samples, std::int64_t pauseNs)
{
    std::vector<PlannedInput> plan;
    std::int64_t start = 0;
    for (int k = 0; k < count; ++k) {
        for (int i = 0; i < samples; ++i) {
            PlannedInput in;
            in.offsetNs = start + i * kStylusPeriodNs;
            in.kind = i == 0             ? InputKind::TabletPress
                      : i == samples - 1 ? InputKind::TabletRelease
                                         : InputKind::TabletMove;
            in.pagePos = QPointF(300.0 + 8.0 * i, 200.0 + 60.0 * k);
            in.pressure = in.kind == InputKind::TabletRelease ? 0.0f : 0.6f;
            in.measured = in.kind != InputKind::TabletRelease;
            plan.push_back(in);
        }
        start += (samples - 1) * kStylusPeriodNs + pauseNs;
    }
    return plan;
}

std::vector<std::pair<std::uint64_t, std::int64_t>> receivedInOrder(std::uint64_t first, std::uint64_t count)
{
    std::vector<std::pair<std::uint64_t, std::int64_t>> out;
    for (std::uint64_t i = 0; i < count; ++i) {
        out.emplace_back(first + i, static_cast<std::int64_t>(i) * kStylusPeriodNs);
    }
    return out;
}

// Vraie interface (ActCraft.Ui/Main), hors écran, en rendu logiciel (tests/CMakeLists.txt).
struct App
{
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::PageView* view = nullptr;
};

bool start(App& app)
{
    StartupOptions options;
    options.generatedPage = true; // le banc mesure la page de 300 objets
    app.window = loadMainWindow(app.engine, options);
    if (!app.window || !QTest::qWaitForWindowExposed(app.window)) {
        return false;
    }
    app.view = app.window->findChild<ui::PageView*>(QStringLiteral("pageView"));
    auto* fixed = app.window->findChild<render::FixedPageItem*>(QStringLiteral("fixedPage"));
    if (!app.view || !fixed) {
        return false;
    }
    return QTest::qWaitFor([&] { return fixed->isReady() && app.view->cacheReady(); }, 20000);
}

// Passage réel du banc (Runner::run, comme bench/app/main.cpp) sur la vraie interface, scénarios courts. Le
// Runner est déclaré AVANT le moteur QML : il doit survivre à la fenêtre (runner.h).
BenchResult runBench(BenchOptions options)
{
    options.scenarioSet = ScenarioSet::Court;
    Runner runner(options);
    QQmlApplicationEngine engine;
    StartupOptions startup;
    startup.generatedPage = true;
    QQuickWindow* window = loadMainWindow(engine, startup);
    if (!window) {
        BenchResult none;
        none.invalidReasons << QStringLiteral("error: interface non chargée");
        return none;
    }
    return runner.run(window);
}

QString describeChecks(const BenchResult& r)
{
    QStringList parts;
    for (const ContentCheck& c : r.contentChecks) {
        parts << QStringLiteral("%1 %2 (lu %3)")
                     .arg(c.name, c.ok ? QStringLiteral("ok") : QStringLiteral("ÉCHEC"), c.actual);
    }
    parts << r.invalidReasons;
    return parts.join(QStringLiteral(" ; "));
}

} // namespace

class TstBenchVerdict : public QObject
{
    Q_OBJECT

private slots:
    void goodRunIsWithinBudget()
    {
        BenchResult r = good();
        applyVerdict(r);
        QVERIFY(r.valid);
        QVERIFY(r.withinBudget);
        QVERIFY(!r.informative);
        QCOMPARE(exitCodeFor(r), ExitCode::WithinBudget);
        QCOMPARE(verdictLine(r), QStringLiteral("BUDGET TENU"));
        QVERIFY(!r.criteria.empty());
    }

    void thresholdsAreInclusiveOrStrictAsSpecified()
    {
        BenchResult r = good();
        r.scenarios[0].intervals.fps = 58.0;
        r.scenarios[0].intervals.active.p95 = 18.5;
        r.scenarios[0].intervals.over25Ratio = 0.01;
        r.scenarios[0].latency->total.p50 = 19.99;
        QCOMPARE(after(r), ExitCode::WithinBudget);

        BenchResult fps = good();
        fps.scenarios[0].intervals.fps = 57.9;
        QCOMPARE(after(fps), ExitCode::OverBudget);
        BenchResult p95 = good();
        p95.scenarios[0].intervals.active.p95 = 18.6;
        QCOMPARE(after(p95), ExitCode::OverBudget);
        BenchResult over = good();
        over.scenarios[0].intervals.over25Ratio = 0.011;
        QCOMPARE(after(over), ExitCode::OverBudget);
        BenchResult latency = good();
        latency.scenarios[0].latency->total.p50 = 20.0; // strict : < 20 ms
        QCOMPARE(after(latency), ExitCode::OverBudget);
        BenchResult freeze = good();
        freeze.scenarios[0].intervals.freezeOverLimit = 1;
        QCOMPARE(after(freeze), ExitCode::OverBudget);
    }

    // H-orch-1 (N1-1) : une reprise hors limite (accroc au début d'un trait, après une pause) compte comme un
    // figement. Sous le verdict qui ne regarde que freezeOverLimit, ce passage serait « BUDGET TENU ».
    void resumesOverLimitAreOverBudget()
    {
        BenchResult r = good();
        r.scenarios[0].intervals.resumeOverLimit = 1;
        r.scenarios[0].intervals.resumeMaxMs = 50;
        applyVerdict(r);
        QCOMPARE(exitCodeFor(r), ExitCode::OverBudget);
        QCOMPARE(verdictLine(r), QStringLiteral("BUDGET DEPASSE"));
        const auto resume = std::find_if(r.criteria.begin(), r.criteria.end(), [](const Criterion& c) {
            return c.name == QLatin1String("resume_over_limit");
        });
        QVERIFY(resume != r.criteria.end());
        QVERIFY(!resume->ok);
        QCOMPARE(resume->value, 1.0);
    }

    // Le test qui distingue : période d'écran inconnue (≤ 0) → classifyIntervals ne calcule rien (0 image/s).
    // Un verdict qui jugerait ces zéros rendrait « BUDGET DEPASSE » (code 1) ; la mesure est invalide (code
    // 2).
    void invalidRefreshPeriodIsInvalidNotOverBudget()
    {
        BenchResult r = good();
        FrameSeries f;
        for (std::int64_t i = 0; i < 100; ++i) {
            f.swapNs.push_back(i * kPeriod60Ns);
            f.inputNs.push_back(i * kPeriod60Ns + 1);
        }
        f.refreshPeriodNs = 0;
        r.scenarios[0].frames = f;
        r.scenarios[0].intervals = classifyIntervals(f);
        QVERIFY(r.scenarios[0].intervals.invalidRefreshPeriod);
        applyVerdict(r);
        QVERIFY(r.invalidReasons.contains(QStringLiteral("invalid-refresh-period")));
        QVERIFY(!r.withinBudget);
        QCOMPARE(exitCodeFor(r), ExitCode::Invalid);
        QCOMPARE(verdictLine(r), QStringLiteral("MESURE INVALIDE"));
        // Aucun critère d'intervalle jugé sur des zéros.
        for (const Criterion& c : r.criteria) {
            QVERIFY2(c.scenario != QLatin1String("stroke") || c.name == QLatin1String("latency_p50_ms"),
                     qPrintable(c.name));
        }
        const QStringList lines = summaryLines(r, QStringLiteral("x.json"));
        QVERIFY(!lines.join('\n').contains(QStringLiteral("Dépassé : stroke fps")));
    }

    void memoryAddsProcessAndGpu()
    {
        BenchResult ok = good();
        ok.scenarios[0].memory.privatePeak = 600 * kMo;
        ok.scenarios[0].memory.gpuPeak = 400 * kMo;
        QCOMPARE(after(ok), ExitCode::WithinBudget);
        BenchResult over = good();
        over.scenarios[0].memory.privatePeak = 700 * kMo;
        over.scenarios[0].memory.gpuPeak = 400 * kMo;
        QCOMPARE(after(over), ExitCode::OverBudget);
    }

    void invalidReasons()
    {
        BenchResult debug = good();
        debug.machine.buildType = QStringLiteral("Debug");
        applyVerdict(debug);
        QVERIFY(debug.invalidReasons.contains(QStringLiteral("build-not-release")));
        QCOMPARE(exitCodeFor(debug), ExitCode::Invalid);
        QCOMPARE(verdictLine(debug), QStringLiteral("MESURE INVALIDE"));

        BenchResult content = good();
        content.contentChecks[0].ok = false;
        applyVerdict(content);
        QVERIFY(content.invalidReasons.contains(QStringLiteral("content-check-failed")));

        BenchResult noCheck = good();
        noCheck.contentChecks.clear();
        QCOMPARE(after(noCheck), ExitCode::Invalid);

        BenchResult exposure = good();
        exposure.scenarios[0].exposureLost = true;
        applyVerdict(exposure);
        QVERIFY(exposure.invalidReasons.contains(QStringLiteral("exposure-lost")));

        BenchResult error = good();
        error.invalidReasons << QStringLiteral("error: injection incomplete in stroke");
        QCOMPARE(after(error), ExitCode::Invalid);
    }

    void reasonsAreNotDuplicated()
    {
        BenchResult r = good();
        r.machine.buildType = QStringLiteral("Debug");
        r.invalidReasons << QStringLiteral("error: essai");
        applyVerdict(r);
        applyVerdict(r);
        QCOMPARE(r.invalidReasons.count(QStringLiteral("build-not-release")), 1);
        QVERIFY(r.invalidReasons.contains(QStringLiteral("error: essai")));
    }

    void softwareIsInformative()
    {
        BenchResult r = good();
        r.graphicsApi = QStringLiteral("software");
        applyVerdict(r);
        QVERIFY(r.informative);
    }

    void summaryEndsWithTheVerdict()
    {
        BenchResult r = good();
        r.scenarios[0].intervals.fps = 40;
        applyVerdict(r);
        const QStringList lines = summaryLines(r, QStringLiteral("x.json"));
        QCOMPARE(lines.last(), QStringLiteral("BUDGET DEPASSE"));
        QVERIFY(lines.join('\n').contains(QStringLiteral("fps")));
    }

    // Période EFFECTIVE (médiane des intervalles d'un rendu continu), pas la période nominale de QScreen : un
    // écran déclaré à 60 Hz qui présente à 144 Hz donne 6,94 ms, et les sauts d'image n'y changent rien.
    void refreshPeriodIsTheMedianInterval()
    {
        std::vector<std::int64_t> swaps;
        std::int64_t t = 1'000'000'000;
        for (int i = 0; i < 40; ++i) {
            swaps.push_back(t);
            t += (i % 9 == 4) ? 2 * 6'944'444 : 6'944'444 + (i % 3) * 50'000; // gigue et images sautées
        }
        const std::int64_t period = measureRefreshPeriodNs(swaps);
        QVERIFY2(period >= 6'944'444 && period <= 6'994'444, qPrintable(QString::number(period)));
    }

    // Trop peu d'images : pas de période plausible par défaut (16,7 ms) ; 0 rend la mesure invalide.
    void tooFewFramesGiveNoRefreshPeriod()
    {
        QCOMPARE(measureRefreshPeriodNs({}), std::int64_t{0});
        std::vector<std::int64_t> swaps;
        for (std::size_t i = 0; i < kMinRefreshIntervals; ++i) {
            swaps.push_back(static_cast<std::int64_t>(i) * kPeriod60Ns);
        }
        QCOMPARE(measureRefreshPeriodNs(swaps), std::int64_t{0}); // kMinRefreshIntervals − 1 intervalles
        swaps.push_back(static_cast<std::int64_t>(kMinRefreshIntervals) * kPeriod60Ns);
        QCOMPARE(measureRefreshPeriodNs(swaps), kPeriod60Ns);
    }

    // Numéro de séquence de l'entrée = indice de l'événement dans le plan de l'injecteur.
    void sequenceMustMatchTheInjectorIndex()
    {
        const auto plan = strokes(2, 10, 50'000'000);
        QVERIFY(sequenceMatchesPlan(plan, receivedInOrder(0, 20)));
        // Numérotation non remise à zéro (trait de démonstration après les scénarios) : décalée.
        QVERIFY(!sequenceMatchesPlan(plan, receivedInOrder(300, 20)));
        // Un événement perdu ou un échantillon de trop.
        QVERIFY(!sequenceMatchesPlan(plan, receivedInOrder(0, 19)));
        QVERIFY(!sequenceMatchesPlan(plan, receivedInOrder(0, 21)));
        // Plan sans tablette : rien ne doit être reçu.
        std::vector<PlannedInput> wheel(3);
        for (PlannedInput& in : wheel) {
            in.kind = InputKind::Wheel;
        }
        QVERIFY(sequenceMatchesPlan(wheel, {}));
        QVERIFY(!sequenceMatchesPlan(wheel, receivedInOrder(0, 1)));
        // Molette au milieu d'un tracé : la chaîne numérote 0, 1, 2 ; le plan, 0, 2, 3.
        std::vector<PlannedInput> mixed = strokes(1, 3, 0);
        PlannedInput w;
        w.kind = InputKind::Wheel;
        mixed.insert(mixed.begin() + 1, w);
        QVERIFY(!sequenceMatchesPlan(mixed, receivedInOrder(0, 3)));
    }

    // Le test qui distingue : si une synchronisation a eu lieu entre le relâché et l'appui suivant sans
    // couvrir le relâché, la fin du trait est rattachée à la première image du trait suivant (latence ≈ la
    // pause).
    void releaseMustBeCoveredByASyncOfItsOwnStroke()
    {
        // Trait 1 : n° 0 à 9 (relâché 9) reçus de 0 à 45 ms ; trait 2 : n° 10 à 19 reçus de 345 à 390 ms ;
        // une synchronisation toutes les 16 ms.
        const auto plan = strokes(2, 10, 300'000'000);
        constexpr std::int64_t ms = 1'000'000;
        ReceivedSamples received;
        for (std::size_t i = 0; i < plan.size(); ++i) {
            received.emplace_back(i, plan[i].offsetNs);
        }
        std::vector<std::int64_t> syncs;
        for (std::int64_t t = 8 * ms; t < 600 * ms; t += 16 * ms) {
            syncs.push_back(t);
        }
        const std::vector<SyncRecord> covered{
            {4, 25 * ms, 30 * ms}, {9, 50 * ms, 55 * ms}, {14, 370 * ms, 375 * ms}, {19, 395 * ms, 400 * ms}};
        const ReleaseCoverage c = releaseCoverage(plan, received, syncs, covered);
        QCOMPARE(c.checked, std::size_t{2});
        QCOMPARE(c.notSynced, std::size_t{0});

        const std::vector<SyncRecord> lateTail{
            {4, 25 * ms, 30 * ms}, {8, 50 * ms, 55 * ms}, {14, 370 * ms, 375 * ms}, {19, 395 * ms, 400 * ms}};
        QCOMPARE(releaseCoverage(plan, received, syncs, lateTail).notSynced, std::size_t{1});
        // Ce que l'invariant protège : le n° 9 irait à la première image du trait suivant (375 ms), soit une
        // « latence » de 330 ms qui n'est que la pause.
        std::vector<std::int64_t> planned(plan.size());
        for (std::size_t i = 0; i < plan.size(); ++i) {
            planned[i] = plan[i].offsetNs;
        }
        const LatencyStats l = computeLatency(planned, lateTail);
        QCOMPARE(l.seq[9], std::uint64_t{9});
        QCOMPARE(l.totalMs[9], 330.0);

        const std::vector<SyncRecord> lastMissing{
            {4, 25 * ms, 30 * ms}, {9, 50 * ms, 55 * ms}, {14, 370 * ms, 375 * ms}};
        QCOMPARE(releaseCoverage(plan, received, syncs, lastMissing).notSynced, std::size_t{1});
        QCOMPARE(releaseCoverage(plan, received, syncs, {}).notSynced, std::size_t{2});

        // Le verdict le juge : la mesure de latence est fausse.
        BenchResult r = good();
        r.scenarios[0].releases = {2, 1};
        applyVerdict(r);
        QVERIFY(r.invalidReasons.contains(QStringLiteral("release-not-synced")));
        QCOMPARE(exitCodeFor(r), ExitCode::Invalid);
    }

    // Point 4 de la revue : une seule synchronisation entre les deux réceptions suffit, elle avait le relâché
    // en attente. La règle « deux images au moins » sautait ce contrôle sur un passage sain et rapide.
    void oneSyncBetweenIsEnoughToCheck()
    {
        const auto plan = strokes(2, 10, 300'000'000);
        constexpr std::int64_t ms = 1'000'000;
        ReceivedSamples received;
        for (std::size_t i = 0; i < plan.size(); ++i) {
            received.emplace_back(i, plan[i].offsetNs);
        }
        const std::vector<std::int64_t> syncs{30 * ms, 200 * ms, 395 * ms, 500 * ms};
        const std::vector<SyncRecord> stale{{8, 30 * ms, 35 * ms}, {19, 395 * ms, 400 * ms}};
        const ReleaseCoverage c = releaseCoverage(plan, received, syncs, stale);
        QCOMPARE(c.checked, std::size_t{2});
        QCOMPARE(c.notSynced, std::size_t{1});
    }

    // Fil principal bloqué de 40 à 400 ms (relevé en debug, rendu logiciel : redessin du cache après un trait
    // figé) : relâché 9 et appui 10 sont reçus d'un bloc, sans synchronisation entre eux ; la première image
    // qui les contient est la même. Ce n'est pas un défaut de mesure mais une lenteur : la latence (≈ 360 ms)
    // va au verdict. Exiger une synchronisation propre au trait 1 rendrait ce passage « MESURE INVALIDE » au
    // lieu de « BUDGET DEPASSE ».
    void releaseReceivedWithTheNextPressIsNotChecked()
    {
        const auto plan = strokes(2, 10, 300'000'000);
        constexpr std::int64_t ms = 1'000'000;
        ReceivedSamples received;
        for (std::size_t i = 0; i < plan.size(); ++i) {
            received.emplace_back(i, i >= 8 ? std::max(plan[i].offsetNs, 400 * ms) : plan[i].offsetNs);
        }
        const std::vector<std::int64_t> syncs{8 * ms, 24 * ms, 36 * ms, 401 * ms, 420 * ms, 436 * ms};
        const std::vector<SyncRecord> records{{7, 36 * ms, 40 * ms}, {19, 401 * ms, 410 * ms}};
        const ReleaseCoverage c = releaseCoverage(plan, received, syncs, records);
        QCOMPARE(c.checked, std::size_t{1}); // le dernier relâché seulement
        QCOMPARE(c.notSynced, std::size_t{0});
    }

    // Point 1 : la référence est la période de l'écran (DWM, sinon QScreen), jamais la médiane du rendu
    // continu, qui suit QT_QPA_UPDATE_IDLE_TIME hors écran (33,4 puis 21,5 ms pour un écran à 60 Hz).
    void refreshReferenceIsTheScreen()
    {
        const RefreshPeriod dwm = referenceRefreshPeriod(16'683'333, 60.0);
        QCOMPARE(dwm.referenceNs, std::int64_t{16'683'333});
        QCOMPARE(dwm.source, QStringLiteral("dwm"));
        const RefreshPeriod screen = referenceRefreshPeriod(0, 144.0);
        QCOMPARE(screen.referenceNs, std::int64_t{6'944'444});
        QCOMPARE(screen.source, QStringLiteral("qscreen"));
        const RefreshPeriod none = referenceRefreshPeriod(0, 0.0);
        QCOMPARE(none.referenceNs, std::int64_t{0});
        QCOMPARE(none.source, QStringLiteral("none"));
    }

    // Hors logiciel, la médiane mesurée doit tenir à 10 % de la référence, et le p50 des intervalles actifs
    // ne peut pas descendre sous 0,9 période (images plus rapides que l'écran : pas de synchronisation
    // verticale).
    void measuredMedianMustMatchTheScreenOutsideSoftware()
    {
        BenchResult ok = good();
        ok.refresh.measuredMedianNs = 18'000'000; // + 8 %
        QCOMPARE(after(ok), ExitCode::WithinBudget);

        BenchResult drift = good();
        drift.refresh.measuredMedianNs = 21'540'000;
        applyVerdict(drift);
        QVERIFY(drift.invalidReasons.contains(QStringLiteral("invalid-refresh-period")));
        QCOMPARE(exitCodeFor(drift), ExitCode::Invalid);

        BenchResult unmeasured = good();
        unmeasured.refresh.measuredMedianNs = 0;
        QCOMPARE(after(unmeasured), ExitCode::Invalid);

        BenchResult fast = good();
        fast.scenarios[0].intervals.active.p50 = 4.9; // 204 images/s sur un écran à 60 Hz
        applyVerdict(fast);
        QVERIFY(fast.invalidReasons.contains(QStringLiteral("invalid-refresh-period")));

        // Logiciel : informatif, la médiane n'est pas celle de l'écran et ne l'invalide pas.
        BenchResult software = good();
        software.graphicsApi = QStringLiteral("software");
        software.refresh.measuredMedianNs = 33'390'000;
        software.scenarios[0].intervals.active.p50 = 4.9;
        software.scenarios[0].memory.gpuPeak.reset();
        applyVerdict(software);
        QVERIFY(!software.invalidReasons.contains(QStringLiteral("invalid-refresh-period")));
    }

    // Point 2 : une sonde muette (aucun échantillon vu dans une image) donnait p50 = 0 → « BUDGET DEPASSE ».
    void unmatchedLatencyIsInvalid()
    {
        BenchResult silent = good();
        silent.scenarios[0].latency = LatencyStats{};
        silent.scenarios[0].latency->unmatched = 590;
        applyVerdict(silent);
        QVERIFY(silent.invalidReasons.contains(QStringLiteral("latency-unmatched")));
        QCOMPARE(exitCodeFor(silent), ExitCode::Invalid);

        BenchResult some = good();
        some.scenarios[0].latency->unmatched = 2;
        QCOMPARE(after(some), ExitCode::Invalid);

        BenchResult empty = good();
        empty.scenarios[0].latency = LatencyStats{};
        QCOMPARE(after(empty), ExitCode::Invalid);
    }

    // Point 3 : hors logiciel, une mémoire graphique illisible comptait pour 0 (N1-7 jugé sur le privé seul).
    void missingGpuMemoryIsInvalidOutsideSoftware()
    {
        BenchResult r = good();
        r.scenarios[0].memory.gpuPeak.reset();
        applyVerdict(r);
        QVERIFY(r.invalidReasons.contains(QStringLiteral("gpu-memory-unavailable")));
        QCOMPARE(exitCodeFor(r), ExitCode::Invalid);

        BenchResult software = good();
        software.graphicsApi = QStringLiteral("software");
        software.scenarios[0].memory.gpuPeak.reset();
        applyVerdict(software);
        QVERIFY(!software.invalidReasons.contains(QStringLiteral("gpu-memory-unavailable")));
        QCOMPARE(exitCodeFor(software), ExitCode::WithinBudget);
    }

    void jsonCarriesTheReferenceAndTheReleaseCheck()
    {
        BenchResult r = good();
        r.scenarios[0].releases = {10, 0};
        const QJsonObject json = toJson(r);
        const QJsonObject refresh = json[QStringLiteral("meta")][QStringLiteral("refresh")].toObject();
        QCOMPARE(refresh[QStringLiteral("source")].toString(), QStringLiteral("dwm"));
        QCOMPARE(refresh[QStringLiteral("reference_period_ms")].toDouble(), 16.666667);
        QCOMPARE(refresh[QStringLiteral("measured_median_ms")].toDouble(), 16.766667);
        const QJsonObject latency =
            json[QStringLiteral("scenarios")].toArray().at(0)[QStringLiteral("latency")].toObject();
        QCOMPARE(latency[QStringLiteral("releases_checked")].toDouble(), 10.0);
        QCOMPARE(latency[QStringLiteral("releases_not_synced")].toDouble(), 0.0);
    }

    // Chaîne réelle : la molette POSTÉE (pas envoyée par sendEvent) atteint la vue, depuis le périphérique de
    // l'injecteur déclaré exclusif comme pendant un scénario.
    void postedWheelReachesThePageView()
    {
        std::unique_ptr<Injector>
            injector; // survit à la fenêtre : déclaré avant elle (main.cpp fait de même)
        App app;
        QVERIFY(start(app));
        injector =
            std::make_unique<Injector>(app.window, [&app](QPointF p) { return app.view->pageToScene(p); });
        app.view->setExclusiveDevices({injector->stylus(), injector->wheel()});

        const qreal before = app.view->zoom();
        PlannedInput zoom;
        zoom.kind = InputKind::Wheel;
        zoom.angleDelta = QPoint(0, 120);
        zoom.modifiers = Qt::ControlModifier;
        injector->start({zoom}, 0, false);
        QCOMPARE(injector->tick(0), std::size_t{1});
        QTRY_VERIFY_WITH_TIMEOUT(!qFuzzyCompare(app.view->zoom(), before), 2000);
        QVERIFY(qFuzzyCompare(app.view->zoom(), before * 1.15));

        app.view->setZoom(4.0);
        app.view->setPan(QPointF(ui::kViewMargin, ui::kViewMargin));
        const QPointF pan = app.view->pan();
        PlannedInput scroll;
        scroll.kind = InputKind::Wheel;
        scroll.angleDelta = QPoint(0, -120);
        injector->start({scroll}, 0, false);
        QCOMPARE(injector->tick(0), std::size_t{1});
        QTRY_VERIFY_WITH_TIMEOUT(app.view->pan() != pan, 2000);
        app.view->setExclusiveDevices({});
    }

    // Chaîne réelle (injecteur → vue → stylo → encre → collecteur) : chaque événement de tablette donne un
    // échantillon numéroté par son indice, et la dernière synchronisation de chaque trait couvre son relâché.
    void realStrokesMatchTheirFrames()
    {
        std::unique_ptr<Injector> injector;
        std::unique_ptr<FrameRecorder> recorder;
        App app;
        QVERIFY(start(app));
        injector =
            std::make_unique<Injector>(app.window, [&app](QPointF p) { return app.view->pageToScene(p); });
        recorder = std::make_unique<FrameRecorder>(app.window);
        app.view->setExclusiveDevices({injector->stylus(), injector->wheel()});
        app.view->resetInputSequence(0); // une seule fois : n° = indice dans `plan` sur les trois traits
        app.view->setInkProbe(recorder.get());

        // Un trait à la fois : le suivant n'est injecté qu'une fois son prédécesseur relâché ET signalé par
        // une synchronisation. Une pause fixe (1 s) ne le garantissait pas : fil principal bloqué plus
        // longtemps (debug, rendu logiciel, tests en parallèle : 8 échecs sur 16), relâché et appui suivant
        // partaient d'un bloc, sans synchronisation entre eux, et l'invariant n'était plus exercé (checked =
        // 2).
        constexpr std::size_t kSamples = 20;
        const auto plan = strokes(3, static_cast<int>(kSamples), 0); // instants recalés trait par trait
        const auto syncedUpTo = [&recorder](std::uint64_t seq) {
            const auto records = recorder->syncRecords();
            return std::any_of(records.begin(), records.end(),
                               [seq](const SyncRecord& rec) { return rec.lastSeq == seq; });
        };
        std::vector<std::int64_t> plannedNs;
        recorder->begin();
        for (std::size_t first = 0; first < plan.size(); first += kSamples) {
            std::vector<PlannedInput> stroke(plan.begin() + static_cast<std::ptrdiff_t>(first),
                                             plan.begin() + static_cast<std::ptrdiff_t>(first + kSamples));
            const std::int64_t shift = stroke.front().offsetNs;
            for (PlannedInput& in : stroke) {
                in.offsetNs -= shift;
            }
            injector->start(stroke, core::monotonicNowNs() + 20'000'000);
            QTRY_VERIFY_WITH_TIMEOUT(injector->finished(), 20000);
            plannedNs.insert(plannedNs.end(), injector->plannedNs().begin(), injector->plannedNs().end());
            // Le relâché (dernier de son trait, seul reçu depuis) est signalé par une synchronisation propre.
            QTRY_VERIFY_WITH_TIMEOUT(syncedUpTo(first + kSamples - 1), 20000);
        }
        recorder->end();
        app.view->setInkProbe(nullptr);
        app.view->setExclusiveDevices({});

        QVERIFY(sequenceMatchesPlan(plan, recorder->received()));
        const auto records = recorder->syncRecords();
        QVERIFY(!records.empty());
        for (const SyncRecord& rec : records) {
            QVERIFY(rec.swapNs >= rec.syncNs); // rattachée au frameSwapped qui suit
        }
        QStringList synced;
        for (const SyncRecord& rec : records) {
            synced << QString::number(rec.lastSeq);
        }
        const ReleaseCoverage coverage =
            releaseCoverage(plan, recorder->received(), recorder->syncStartNs(), records);
        QCOMPARE(coverage.checked, std::size_t{3}); // une synchronisation suit chaque relâché
        QVERIFY2(coverage.notSynced == 0, qPrintable(synced.join(' ')));
        QCOMPARE(plannedNs.size(), plan.size());
        std::vector<std::int64_t> measured(plan.size(), -1);
        for (std::size_t i = 0; i < plan.size(); ++i) {
            if (plan[i].measured) {
                measured[i] = plannedNs[i];
            }
        }
        const LatencyStats latency = computeLatency(measured, records);
        QCOMPARE(latency.unmatched, std::size_t{0});
        QCOMPARE(latency.seq.size(), std::size_t{57});
        QVERIFY(!recorder->swapNs().empty());
    }

    // Chemin réel (Runner::run → runDemo) à 1366 × 768 (Dell 3510) : la démonstration recentre la vue sur ses
    // points contrôlés. Si runDemo se contentait de setZoom(1.0), la vue garderait le centre laissé par le
    // défilement (400 %, haut à gauche) et la sonde du témoin de trait sortirait de la fenêtre (« hors-image
    // »). Une taille LxH explicite l'emporte sur la taille fixe hors écran.
    void benchRunReadsItsTargetsAt1366x768()
    {
        BenchOptions options;
        options.fullScreen = false;
        options.windowSize = QSize(1366, 768);
        const BenchResult r = runBench(options);
        QCOMPARE(r.windowMode, QStringLiteral("window"));
        QCOMPARE(r.windowSize, QSize(1366, 768));
        QCOMPARE(r.contentChecks.size(), std::size_t{5}); // L10-T4 : stroke-folded
        const bool allOk = std::all_of(r.contentChecks.begin(), r.contentChecks.end(),
                                       [](const ContentCheck& c) { return c.ok; });
        QVERIFY2(allOk, qPrintable(describeChecks(r)));
    }

    // Hors écran, le plein écran demandé (défaut du banc) donne une fenêtre fixe de 1280 × 800, pas l'écran
    // fictif de la plateforme (800 de large : témoins hors de la vue, content-check-failed).
    void offscreenFullScreenGetsTheFixedSize()
    {
        QCOMPARE(QGuiApplication::platformName(), QStringLiteral("offscreen"));
        BenchOptions options;
        QVERIFY(options.fullScreen);
        const BenchResult r = runBench(options);
        QCOMPARE(r.windowMode, QStringLiteral("window"));
        QCOMPARE(r.windowSize, QSize(1280, 800));
    }

    void benchTargetsAreVisibleInTheShell_data()
    {
        QTest::addColumn<QSize>("size");
        QTest::newRow("1366x768") << QSize(1366, 768); // Dell 3510, machine de référence du BRIEF
        QTest::newRow("1280x800") << QSize(1280, 800); // fumée et preuves hors écran
        QTest::newRow("1280x720") << QSize(1280, 720);
        QTest::newRow("1920x1080") << QSize(1920, 1080);
    }

    // DA7 : dans la nouvelle interface, les points contrôlés par la démonstration du banc tombent dans la vue
    // à 100 %, hors barre, palette et barres de défilement visibles, quelle que soit la position laissée par
    // les scénarios : le défilement finit à 400 %, calé en haut à gauche (scenarios.cpp, scrollScenario).
    void benchTargetsAreVisibleInTheShell()
    {
        QFETCH(QSize, size);
        App app;
        QVERIFY(start(app));
        app.window->resize(size);
        QTRY_COMPARE(app.window->size(), size);
        // La vue prend sa taille au polissage de la disposition, après la fenêtre : la positionner avant,
        // c'est centrer sur l'ancienne hauteur. Le banc, lui, la positionne après les scénarios.
        const auto* palette = app.window->findChild<QQuickItem*>(QStringLiteral("toolPalette"));
        QVERIFY(palette);
        QTRY_VERIFY(qRound(app.view->mapRectToScene(app.view->boundingRect()).bottom()) == size.height() &&
                    qRound(palette->mapRectToScene(palette->boundingRect()).right()) == size.width());
        app.view->setZoom(4.0);
        app.view->setPan(QPointF(ui::kViewMargin, ui::kViewMargin));
        showDemoTargets(*app.view);      // ce que fait runDemo, au début et au retour à 100 %
        QCOMPARE(app.view->zoom(), 1.0); // 100 % : un ajustement de la page à la vue ne compterait pas
        QTRY_VERIFY(app.view->cacheReady());
        const QRectF visible = app.view->mapRectToScene(app.view->boundingRect());
        QList<QRectF> bars; // posées sur la vue : un point dessous n'est pas lisible
        for (const char* name : {"horizontalScrollBar", "verticalScrollBar"}) {
            const auto* bar = app.window->findChild<QQuickItem*>(QLatin1String(name));
            QVERIFY2(bar, name);
            if (bar->isVisible()) {
                bars << bar->mapRectToScene(bar->boundingRect());
            }
        }
        const core::Witnesses& w = core::witnesses();
        const QList<QPointF> targets{
            QPointF(w.shapeRect.center().x, w.shapeRect.center().y),
            QPointF(w.imageRect.center().x, w.imageRect.center().y),
            QPointF(w.strokeProbe.x, w.strokeProbe.y),
            bench::demoInkProbePoint(),
        };
        for (const QPointF& page : targets) {
            const QPointF scene = app.view->pageToScene(page);
            const bool underBar = std::any_of(bars.begin(), bars.end(),
                                              [&scene](const QRectF& bar) { return bar.contains(scene); });
            QVERIFY2(
                visible.contains(scene) && !underBar,
                qPrintable(QStringLiteral("point de page (%1, %2) → scène (%3, %4) hors de la vue (%5, %6, "
                                          "%7 × %8) ou sous une barre de défilement")
                               .arg(page.x())
                               .arg(page.y())
                               .arg(scene.x())
                               .arg(scene.y())
                               .arg(visible.x())
                               .arg(visible.y())
                               .arg(visible.width())
                               .arg(visible.height())));
        }
    }
};

QTEST_MAIN(TstBenchVerdict)
#include "tst_bench_verdict.moc"
