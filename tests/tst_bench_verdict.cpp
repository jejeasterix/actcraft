#include "startup.h"

#include <actcraft/bench/injector.h>
#include <actcraft/bench/recorder.h>
#include <actcraft/bench/verdict.h>
#include <actcraft/core/clock.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/pageview.h>

#include <QQmlApplicationEngine>
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
    ScenarioResult s;
    s.name = QStringLiteral("stroke");
    s.frames.refreshPeriodNs = kPeriod60Ns;
    s.intervals.fps = 60;
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
    app.window = loadMainWindow(app.engine, StartupOptions{});
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

    // Le test qui distingue : si la dernière synchronisation d'un trait ne couvre pas son relâché alors que
    // des images ont été synchronisées avant l'appui suivant, la fin du trait est rattachée à la première
    // image du trait suivant (latence ≈ la pause).
    void releaseMustBeCoveredByASyncOfItsOwnStroke()
    {
        // Trait 1 : n° 0 à 9 (relâché 9) reçus de 0 à 45 ms ; trait 2 : n° 10 à 19 reçus de 345 à 390 ms ;
        // une image toutes les 16 ms.
        const auto plan = strokes(2, 10, 300'000'000);
        constexpr std::int64_t ms = 1'000'000;
        ReceivedSamples received;
        for (std::size_t i = 0; i < plan.size(); ++i) {
            received.emplace_back(i, plan[i].offsetNs);
        }
        std::vector<std::int64_t> swaps;
        for (std::int64_t t = 8 * ms; t < 600 * ms; t += 16 * ms) {
            swaps.push_back(t);
        }
        const std::vector<SyncRecord> covered{
            {4, 25 * ms, 30 * ms}, {9, 50 * ms, 55 * ms}, {14, 370 * ms, 375 * ms}, {19, 395 * ms, 400 * ms}};
        QCOMPARE(releasesNotSynced(plan, received, swaps, covered), std::size_t{0});

        const std::vector<SyncRecord> lateTail{
            {4, 25 * ms, 30 * ms}, {8, 50 * ms, 55 * ms}, {14, 370 * ms, 375 * ms}, {19, 395 * ms, 400 * ms}};
        QCOMPARE(releasesNotSynced(plan, received, swaps, lateTail), std::size_t{1});
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
        QCOMPARE(releasesNotSynced(plan, received, swaps, lastMissing), std::size_t{1});
        QCOMPARE(releasesNotSynced(plan, received, swaps, {}), std::size_t{2});
    }

    // Fil principal bloqué de 40 à 400 ms (relevé en debug, rendu logiciel : redessin du cache après un trait
    // figé) : relâché 9 et appui 10 sont reçus d'un bloc, la première image qui les contient est la même. Ce
    // n'est pas un défaut de mesure mais une lenteur : la latence (≈ 360 ms) va au verdict, qui la juge.
    // Exiger une synchronisation propre au trait 1 rendrait ce passage « MESURE INVALIDE » au lieu de «
    // BUDGET DEPASSE ».
    void releaseReceivedWithTheNextPressIsNotAMeasurementDefect()
    {
        const auto plan = strokes(2, 10, 300'000'000);
        constexpr std::int64_t ms = 1'000'000;
        ReceivedSamples received;
        for (std::size_t i = 0; i < plan.size(); ++i) {
            received.emplace_back(i, i >= 8 ? std::max(plan[i].offsetNs, 400 * ms) : plan[i].offsetNs);
        }
        const std::vector<std::int64_t> swaps{8 * ms, 24 * ms, 40 * ms, 410 * ms, 426 * ms, 442 * ms};
        const std::vector<SyncRecord> records{
            {7, 36 * ms, 40 * ms}, {11, 401 * ms, 410 * ms}, {19, 420 * ms, 426 * ms}};
        QCOMPARE(releasesNotSynced(plan, received, swaps, records), std::size_t{0});
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
        app.view->resetInputSequence(0);
        app.view->setInkProbe(recorder.get());

        // Pauses longues : en debug et rendu logiciel, le redessin du cache après un trait figé bloque le fil
        // principal plusieurs centaines de ms ; il faut des images entre relâché et appui suivant pour que
        // l'invariant soit exercé.
        const auto plan = strokes(3, 20, 800'000'000);
        recorder->begin();
        injector->start(plan, core::monotonicNowNs() + 20'000'000);
        QTRY_VERIFY_WITH_TIMEOUT(injector->finished(), 20000);
        QTest::qWait(300);
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
        QVERIFY2(releasesNotSynced(plan, recorder->received(), recorder->swapNs(), records) == 0,
                 qPrintable(synced.join(' ')));
        // Premier trait, précédé d'un repos : son relâché (n° 19) est signalé par une synchronisation propre.
        QVERIFY2(std::any_of(records.begin(), records.end(),
                             [](const SyncRecord& rec) { return rec.lastSeq == 19; }),
                 qPrintable(synced.join(' ')));
        std::vector<std::int64_t> measured(plan.size(), -1);
        for (std::size_t i = 0; i < plan.size(); ++i) {
            if (plan[i].measured) {
                measured[i] = injector->plannedNs()[i];
            }
        }
        const LatencyStats latency = computeLatency(measured, records);
        QCOMPARE(latency.unmatched, std::size_t{0});
        QCOMPARE(latency.seq.size(), std::size_t{57});
        QVERIFY(!recorder->swapNs().empty());
    }
};

QTEST_MAIN(TstBenchVerdict)
#include "tst_bench_verdict.moc"
