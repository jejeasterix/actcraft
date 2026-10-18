#include <actcraft/bench/result.h>
#include <actcraft/bench/scenarios.h>
#include <actcraft/core/generator.h>
#include <actcraft/render/paintedrenderer.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/viewtransform.h>

#include <QTest>

#include <cmath>
#include <limits>
#include <numeric>
#include <set>

using namespace actcraft;
using bench::InputKind;

class TstBenchScenarios : public QObject
{
    Q_OBJECT

private slots:
    void strokeScenarioShape()
    {
        const bench::ScenarioPlan plan = bench::strokeScenario(bench::ScenarioSet::Complet);
        QCOMPARE(plan.name, QStringLiteral("stroke"));
        QCOMPARE(plan.zoom, 1.0);
        QCOMPARE(plan.inputs.size(), std::size_t{3000});
        int presses = 0;
        int releases = 0;
        for (std::size_t i = 0; i < plan.inputs.size(); ++i) {
            const auto& in = plan.inputs[i];
            presses += in.kind == InputKind::TabletPress ? 1 : 0;
            releases += in.kind == InputKind::TabletRelease ? 1 : 0;
            QCOMPARE(in.measured, in.kind != InputKind::TabletRelease);
            if (i > 0) {
                QVERIFY(in.offsetNs > plan.inputs[i - 1].offsetNs);
                const std::int64_t gap = in.offsetNs - plan.inputs[i - 1].offsetNs;
                QVERIFY(gap == bench::kStylusPeriodNs ||
                        (in.kind == InputKind::TabletPress && gap >= 300'000'000));
            }
        }
        QCOMPARE(presses, 10);
        QCOMPARE(releases, 10);
    }

    void strokesStayInTheCentralZone()
    {
        for (const auto& in : bench::strokeScenario(bench::ScenarioSet::Complet).inputs) {
            QVERIFY(!core::hitsWitness({in.pagePos.x(), in.pagePos.y()}, 16.0));
            QVERIFY(in.pagePos.y() >= 149.0 && in.pagePos.y() <= 619.0);
            QVERIFY(in.pagePos.x() >= 200.0 && in.pagePos.x() <= 824.0);
        }
    }

    void courtDividesByFive()
    {
        const auto complet = bench::strokeScenario(bench::ScenarioSet::Complet);
        const auto court = bench::strokeScenario(bench::ScenarioSet::Court);
        QCOMPARE(court.inputs.size(), std::size_t{600});
        const double ratio = static_cast<double>(bench::planDurationNs(complet)) /
                             static_cast<double>(bench::planDurationNs(court));
        QVERIFY(ratio > 4.8 && ratio < 5.2);
        QCOMPARE(bench::zoomScenario(bench::ScenarioSet::Court).inputs.size(), std::size_t{144});
        QCOMPARE(bench::scrollScenario(bench::ScenarioSet::Court).inputs.size(), std::size_t{144});
    }

    // Rejoue les deltas avec les fonctions de la vue (PageView::wheelEvent) : pas avec 1,15 par événement,
    // comme la référence Python (N2-11), qui ferait de chaque cran fin un cran entier.
    void zoomScenarioReachesItsTargets()
    {
        const auto plan = bench::zoomScenario(bench::ScenarioSet::Complet);
        QCOMPARE(plan.inputs.size(), std::size_t{720});
        double zoom = 1.0;
        double highest = 1.0;
        double lowest = 1.0;
        for (const auto& in : plan.inputs) {
            QCOMPARE(in.kind, InputKind::Wheel);
            QVERIFY(in.modifiers & Qt::ControlModifier);
            zoom = ui::clampZoom(zoom * ui::wheelZoomFactor(in.angleDelta.y()));
            highest = std::max(highest, zoom);
            lowest = std::min(lowest, zoom);
        }
        QVERIFY(std::abs(highest - 8.0) < 0.08);
        QVERIFY(std::abs(lowest - 0.25) < 0.0025);
        QVERIFY(std::abs(zoom - 1.0) < 0.01);
        const double seconds = static_cast<double>(bench::planDurationNs(plan)) / 1e9;
        QVERIFY(seconds > 5.9 && seconds < 6.1);
    }

    void scrollScenarioLegs()
    {
        const auto plan = bench::scrollScenario(bench::ScenarioSet::Complet);
        QCOMPARE(plan.zoom, 4.0);
        QVERIFY(plan.topLeft);
        QCOMPARE(plan.inputs.size(), std::size_t{720});
        QCOMPARE(plan.inputs[0].angleDelta, QPoint(0, -20));
        QCOMPARE(plan.inputs[180].angleDelta, QPoint(0, 20));
        QCOMPARE(plan.inputs[360].angleDelta, QPoint(-20, 0));
        QCOMPARE(plan.inputs[540].angleDelta, QPoint(20, 0));
        QCOMPARE(plan.inputs[0].modifiers, Qt::KeyboardModifiers(Qt::NoModifier));
    }

    // Zoom et défilement s'enchaînent sans pause (décision 22) : la pause stylo levé du tracé, reprise ici,
    // ferait sortir des intervalles de la mesure (décision 24) sans que la durée totale le trahisse.
    void wheelScenariosChainWithoutPause()
    {
        for (const auto set : {bench::ScenarioSet::Complet, bench::ScenarioSet::Court}) {
            for (const auto& plan : {bench::zoomScenario(set), bench::scrollScenario(set)}) {
                QCOMPARE(plan.inputs.front().offsetNs, std::int64_t{0});
                for (std::size_t i = 1; i < plan.inputs.size(); ++i) {
                    QCOMPARE(plan.inputs[i].offsetNs - plan.inputs[i - 1].offsetNs, bench::kWheelPeriodNs);
                    QVERIFY(!plan.inputs[i].measured);
                }
            }
        }
    }

    void allScenariosInOrder()
    {
        const auto plans = bench::allScenarios(bench::ScenarioSet::Court);
        QCOMPARE(plans.size(), std::size_t{5}); // L2-T23 : « page » ; L10-T1 : « pauses », en dernier
        QCOMPARE(plans[0].name, QStringLiteral("stroke"));
        QCOMPARE(plans[1].name, QStringLiteral("zoom"));
        QCOMPARE(plans[2].name, QStringLiteral("scroll"));
        QCOMPARE(plans[3].name, QStringLiteral("page"));
        QCOMPARE(plans[4].name, QStringLiteral("pauses"));
    }

    // L10-T1 (DA11) : chaque reprise tombe après le délai d'inactivité du cache, décalée de 0 à 64 ms (court
    // : 0 à 48), pour tomber dans la fenêtre du redessin différé ; page du banc rechargée (300 objets).
    void pausesResumeInsideTheFoldWindow()
    {
        for (const auto set : {bench::ScenarioSet::Complet, bench::ScenarioSet::Court}) {
            const bench::ScenarioPlan plan = bench::pausesScenario(set);
            QCOMPARE(plan.name, QStringLiteral("pauses"));
            QCOMPARE(plan.generatedPages, 1);
            QCOMPARE(plan.zoom, 1.0);
            std::vector<std::int64_t> pauses;
            std::int64_t lastRelease = -1;
            for (const auto& in : plan.inputs) {
                QVERIFY(!core::hitsWitness({in.pagePos.x(), in.pagePos.y()}, 16.0));
                QCOMPARE(in.measured, in.kind != InputKind::TabletRelease);
                if (in.kind == InputKind::TabletPress && lastRelease >= 0) {
                    pauses.push_back(in.offsetNs - lastRelease);
                }
                if (in.kind == InputKind::TabletRelease) {
                    lastRelease = in.offsetNs;
                }
            }
            const std::size_t strokes = set == bench::ScenarioSet::Court ? 4 : 10;
            QCOMPARE(pauses.size(), strokes - 1);
            const std::int64_t idleNs = std::int64_t{ui::kCacheFoldIdleMs} * 1'000'000;
            for (std::int64_t p : pauses) {
                QVERIFY(p >= idleNs);
                QVERIFY(p <= idleNs + 64'000'000);
            }
            // k = 0 : l'appui tombe à l'instant du versement et relance la minuterie ; il ne mesure rien de
            // sûr. Seules les reprises k >= 1 jugent le versement (L10-T6).
            QCOMPARE(pauses.front(), idleNs);
            QVERIFY(pauses.back() - pauses.front() >= 40'000'000); // la fenêtre du versement est balayée
        }
    }

    void pausesComesAfterTheOthersWithoutChangingThem()
    {
        const auto plans = bench::allScenarios(bench::ScenarioSet::Complet);
        QCOMPARE(plans[0].inputs.size(), bench::strokeScenario(bench::ScenarioSet::Complet).inputs.size());
        QCOMPARE(plans[4].inputs.size(), std::size_t{10 * 100});
    }

    // L2-T23 : changements de page à intervalle fixe par commandes nommées (aucun événement posté, aucune
    // encre), puis deux zooms posés sur la page peinte ; document de deux pages générées chargé avant.
    void pageScenarioFlipsPages()
    {
        const bench::ScenarioPlan court = bench::pageScenario(bench::ScenarioSet::Court);
        QCOMPARE(court.name, QStringLiteral("page"));
        QCOMPARE(court.generatedPages, 2);
        QCOMPARE(court.inputs.size(), std::size_t{7});
        for (std::size_t i = 0; i < 4; ++i) {
            const bench::PlannedInput& in = court.inputs[i];
            QVERIFY(in.kind == InputKind::Command);
            QCOMPARE(in.command, i % 2 == 0 ? QStringLiteral("page.next") : QStringLiteral("page.previous"));
            QCOMPARE(in.offsetNs, static_cast<std::int64_t>(i) * 500'000'000);
            QVERIFY(!in.argument.isValid());
            QVERIFY(!in.measured);
        }
        const QStringList zooms{QStringLiteral("page.next"), QStringLiteral("view.zoom"),
                                QStringLiteral("view.fit")};
        for (std::size_t k = 0; k < 3; ++k) {
            const bench::PlannedInput& in = court.inputs[4 + k];
            QVERIFY(in.kind == InputKind::Command);
            QCOMPARE(in.command, zooms.at(static_cast<qsizetype>(k)));
            QCOMPARE(in.offsetNs, 2'000'000'000 + static_cast<std::int64_t>(k) * 700'000'000);
            QVERIFY(!in.measured);
        }
        QVERIFY(!court.inputs[4].argument.isValid());
        QCOMPARE(court.inputs[5].argument.toInt(), 150);
        QVERIFY(!court.inputs[6].argument.isValid());

        const bench::ScenarioPlan complet = bench::pageScenario(bench::ScenarioSet::Complet);
        QCOMPARE(complet.inputs.size(), std::size_t{23});
        QCOMPARE(complet.inputs[19].command,
                 QStringLiteral("page.previous"));                         // 20 changements : retour page 1
        QCOMPARE(complet.inputs[20].command, QStringLiteral("page.next")); // puis la page peinte
        QCOMPARE(complet.inputs.back().command, QStringLiteral("view.fit"));
        // Les scénarios de référence gardent la page en place.
        for (const auto& plan : {bench::strokeScenario(bench::ScenarioSet::Court),
                                 bench::zoomScenario(bench::ScenarioSet::Court),
                                 bench::scrollScenario(bench::ScenarioSet::Court)}) {
            QCOMPARE(plan.generatedPages, 0);
            for (const bench::PlannedInput& in : plan.inputs) {
                QVERIFY(in.kind != InputKind::Command);
            }
        }
    }

    // Revue L2-T23 : un changement de page compte, de sa commande à la première image après le prêt, comme
    // une suite d'intervalles actifs ; sans ces marques, seul l'intervalle de la commande l'était.
    void pageChangeIntervalsAreActive()
    {
        bench::FrameSeries s;
        s.refreshPeriodNs = 16'000'000;
        for (std::int64_t k = 0; k <= 10; ++k) {
            s.swapNs.push_back(k * 16'000'000); // 0, 16, …, 160 ms
        }
        s.inputNs = {20'000'000}; // commande dans (16, 32]
        const std::vector<std::int64_t> marks = bench::pageChangeMarks(s.swapNs, {{20'000'000, 70'000'000}});
        QCOMPARE(marks, (std::vector<std::int64_t>{
                            16'000'001, 32'000'001, 48'000'001,
                            64'000'001})); // de l'intervalle de la commande à l'image de 80 ms
        QCOMPARE(static_cast<int>(bench::classifyIntervals(s).active.count), 1);
        bench::FrameSeries busy = s;
        busy.inputNs.insert(busy.inputNs.end(), marks.begin(), marks.end());
        std::sort(busy.inputNs.begin(), busy.inputNs.end());
        QCOMPARE(static_cast<int>(bench::classifyIntervals(busy).active.count), 4);
        // Minuteur en retard (2e revue) : prévue à 5 ms, la commande s'exécute à 20 ms ; l'intervalle (16,
        // 32] où elle tourne est actif, pas seulement celui de son heure prévue.
        bench::FrameSeries late = s;
        late.inputNs = {5'000'000};
        const std::vector<std::int64_t> lateMarks =
            bench::pageChangeMarks(s.swapNs, {{20'000'000, 20'000'000}});
        QCOMPARE(lateMarks, (std::vector<std::int64_t>{16'000'001}));
        late.inputNs.insert(late.inputNs.end(), lateMarks.begin(), lateMarks.end());
        QCOMPARE(static_cast<int>(bench::classifyIntervals(late).active.count), 2);
        // Jamais prête pendant la mesure : tout ce qui suit la commande est actif.
        QCOMPARE(
            bench::pageChangeMarks(s.swapNs, {{20'000'000, std::numeric_limits<std::int64_t>::max()}}).size(),
            std::size_t{9});
    }

    // 2e revue L2-T23 : une commande qui n'attend aucun prêt (zoom sans cache : rien n'émet readyChanged ni
    // cacheReadyChanged) ferme sa fenêtre à elle-même ; sinon elle courrait jusqu'au prêt suivant ou à la
    // fin.
    void pageChangeWindowsCloseWithoutAwaitedReady()
    {
        constexpr std::int64_t kNever = std::numeric_limits<std::int64_t>::max();
        using Window = std::pair<std::int64_t, std::int64_t>;
        const std::vector<Window> windows =
            bench::pageChangeWindows({20, 600, 1300, -1, 1400}, {true, true, false, true, true}, {70, 650});
        QCOMPARE(windows, (std::vector<Window>{{20, 70}, {600, 650}, {1300, 1300}, {1400, kNever}}));
    }

    // L2-T23 : document du scénario « page » : deux pages générées, la seconde peinte (20 textes, 20 formes à
    // chemins), identifiants uniques dans le document, état propre comme une ouverture, page 1 affichée.
    void generatedPagesForThePageScenario()
    {
        ui::DocumentController controller;
        controller.loadGeneratedPages(2, true);
        const core::Document& d = controller.document();
        QCOMPARE(d.pageCount(), std::size_t{2});
        const auto painted = [](const core::Page& page) {
            int texts = 0;
            int paths = 0;
            for (const core::PageObject& object : page.objects) {
                texts += std::get_if<core::Text>(&object) ? 1 : 0;
                const auto* shape = std::get_if<core::Shape>(&object);
                paths += shape && shape->kind == core::ShapeKind::Path ? 1 : 0;
            }
            return std::make_pair(texts, paths);
        };
        const core::Page first = core::generatePage(core::kDefaultSeed);
        const core::Page second = core::generatePage(core::kDefaultSeed + 1);
        QCOMPARE(d.page(0).objects.size(), first.objects.size());
        QCOMPARE(d.page(1).objects.size(), second.objects.size() + 40);
        QCOMPARE(painted(d.page(0)), painted(first));
        QCOMPARE(painted(d.page(1)).first, painted(second).first + 20);
        QCOMPARE(painted(d.page(1)).second, painted(second).second + 20);
        std::set<core::ObjectId> ids;
        for (std::size_t p = 0; p < d.pageCount(); ++p) {
            for (const core::PageObject& object : d.page(p).objects) {
                QVERIFY(ids.insert(object.common.id).second);
            }
        }
        QVERIFY(!controller.isModified());
        QVERIFY(!controller.canUndo());
        QCOMPARE(controller.currentIndex(), 0);
        // Valeur attendue du relevé D3D11 (docs/banc.md) : la page 2 a exactement 40 nœuds rastérisés en RHI
        // (éléments peints, ni fond peint ni grille), la page 1 aucun. Deux échelles posées (view.zoom,
        // view.fit) : page_rasterizations_during_zoom = 2 × 40 = 80.
        const auto rasterSlots = [](const core::Page& page) {
            int count = render::backgroundIsPainted(page) ? 1 : 0;
            count += page.grid.visible && !page.grid.levels.empty() ? 1 : 0;
            for (const core::PageObject& object : page.objects) {
                count += object.common.visible && render::usesPaintedRenderer(object) ? 1 : 0;
            }
            return count;
        };
        QCOMPARE(rasterSlots(d.page(0)), 0);
        QCOMPARE(rasterSlots(d.page(1)), 40);
    }

    void cumulativeSplitIsExact()
    {
        const std::vector<int> parts = bench::cumulativeSplit(1785, 216);
        QCOMPARE(parts.size(), std::size_t{216});
        QCOMPARE(std::accumulate(parts.begin(), parts.end(), 0), 1785);
        const std::vector<int> negative = bench::cumulativeSplit(-2976, 360);
        QCOMPARE(std::accumulate(negative.begin(), negative.end(), 0), -2976);
    }

    void demoStrokeIsOutsideTheMeasuredZone()
    {
        const auto demo = bench::demoStroke();
        QVERIFY(demo.size() > bench::kDemoMidIndex + 10);
        QCOMPARE(demo.front().kind, InputKind::TabletPress);
        QCOMPARE(demo.back().kind, InputKind::TabletRelease);
        for (const auto& in : demo) {
            QVERIFY(!in.measured);
            QVERIFY(in.pagePos.y() > 640.0 && in.pagePos.y() < 700.0);
            QVERIFY(!core::hitsWitness({in.pagePos.x(), in.pagePos.y()}, 16.0));
        }
        const QPointF probe = bench::demoInkProbePoint();
        QVERIFY(probe.x() < demo[bench::kDemoMidIndex - 1].pagePos.x()); // déjà tracé à la capture mi-trait
    }
};

QTEST_GUILESS_MAIN(TstBenchScenarios)
#include "tst_bench_scenarios.moc"
