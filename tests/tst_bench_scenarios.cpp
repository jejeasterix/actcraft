#include <actcraft/bench/scenarios.h>
#include <actcraft/core/generator.h>
#include <actcraft/ui/viewtransform.h>

#include <QTest>

#include <cmath>
#include <numeric>

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
        QCOMPARE(plans.size(), std::size_t{4}); // L2-T23 : scénario « page » ajouté en dernier
        QCOMPARE(plans[0].name, QStringLiteral("stroke"));
        QCOMPARE(plans[1].name, QStringLiteral("zoom"));
        QCOMPARE(plans[2].name, QStringLiteral("scroll"));
        QCOMPARE(plans[3].name, QStringLiteral("page"));
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
