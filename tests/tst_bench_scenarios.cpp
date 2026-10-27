#include <actcraft/bench/result.h>
#include <actcraft/bench/scenarios.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/generator.h>
#include <actcraft/core/hittest.h>
#include <actcraft/render/paintedrenderer.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pagethumbnails.h>
#include <actcraft/ui/viewtransform.h>

#include <QTest>

#include <algorithm>
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
        // L2-T23 : « page » ; L10-T1 : « pauses » ; L3-T19 : « gommer » et « soulever », en dernier (le
        // compte passe de 5 à 7 : les cinq premiers gardent leur nom et leur ordre). L4-T14 : « forme-page-
        // annotee » et « grille-dessus » après eux (le compte passe de 7 à 9 : les sept premiers inchangés).
        // L4-T29 : « miniatures » et « miniatures-ferme » après eux (le compte passe de 9 à 11), puis « miniatures-pauses » (12).
        QCOMPARE(plans.size(), std::size_t{12});
        QCOMPARE(plans[0].name, QStringLiteral("stroke"));
        QCOMPARE(plans[1].name, QStringLiteral("zoom"));
        QCOMPARE(plans[2].name, QStringLiteral("scroll"));
        QCOMPARE(plans[3].name, QStringLiteral("page"));
        QCOMPARE(plans[4].name, QStringLiteral("pauses"));
        QCOMPARE(plans[5].name, QStringLiteral("gommer"));
        QCOMPARE(plans[6].name, QStringLiteral("soulever"));
        QCOMPARE(plans[7].name, QStringLiteral("forme-page-annotee"));
        QCOMPARE(plans[8].name, QStringLiteral("grille-dessus"));
        QCOMPARE(plans[9].name, QStringLiteral("miniatures"));
        QCOMPARE(plans[10].name, QStringLiteral("miniatures-ferme"));
        QCOMPARE(plans[11].name, QStringLiteral("miniatures-pauses"));
        for (std::size_t i = 0; i < 5; ++i) {
            QVERIFY(plans[i].toolCommand.isEmpty()); // les scénarios existants gardent le stylo
            QVERIFY(plans[i].seedStrokes.empty());
        }
    }

    // L3-T19 (S36) : la gomme ne touche que les annotations (couche supérieure) ; les traits de la page
    // générée sont sur la couche du milieu. Le scénario pose donc ses propres annotations, versées avant la
    // mesure, et le zigzag du stylo en croise au moins 20.
    // L4-T29 (S7.5, amendements 1 et 5) : « miniatures » joue les traits de « stroke » panneau des
    // navigateurs ouvert (view.browsers avant la mesure, refermé après), sur la page générée et trois pages
    // de plus pour que le navigateur ait des miniatures à tenir à jour.
    void thumbnailsScenarioOpensTheBrowsers()
    {
        for (const auto set : {bench::ScenarioSet::Complet, bench::ScenarioSet::Court}) {
            const bench::ScenarioPlan plan = bench::thumbnailsScenario(set);
            const bench::ScenarioPlan stroke = bench::strokeScenario(set);
            QCOMPARE(plan.name, QStringLiteral("miniatures"));
            QCOMPARE(plan.generatedPages, 4);
            QVERIFY(plan.measuresThumbnails);
            QVERIFY(plan.browsersOpen);
            QCOMPARE(plan.setupCommands.size(), std::size_t{1});
            QCOMPARE(plan.setupCommands[0].first, QStringLiteral("view.browsers"));
            QCOMPARE(plan.teardownCommands.size(), std::size_t{1});
            QCOMPARE(plan.teardownCommands[0].first, QStringLiteral("view.browsers"));
            QVERIFY(plan.toolCommand.isEmpty()); // le stylo
            QVERIFY(!plan.freezeExempt);         // jugé comme « stroke », figements compris
            QCOMPARE(plan.maxCacheRefreshes, -1);
            QVERIFY(!plan.awaitFinalFold);
            QCOMPARE(plan.inputs.size(), stroke.inputs.size());
            for (std::size_t i = 0; i < plan.inputs.size(); ++i) {
                QCOMPARE(plan.inputs[i].offsetNs, stroke.inputs[i].offsetNs);
                QCOMPARE(plan.inputs[i].pagePos, stroke.inputs[i].pagePos);
                QCOMPARE(plan.inputs[i].kind, stroke.inputs[i].kind);
                QCOMPARE(plan.inputs[i].measured, stroke.inputs[i].measured);
            }
        }
    }

    // L4-T29 (amendement 1) : variante sur « pauses » : les traits de « pauses » joués panneau ouvert, pour
    // que les rendus (400 ms après la dernière modification) tombent dans les pauses ; ses figements sont
    // comparés à ceux de « pauses » sans panneau, dans le même JSON.
    void thumbnailsPausesVariant()
    {
        for (const auto set : {bench::ScenarioSet::Complet, bench::ScenarioSet::Court}) {
            const bench::ScenarioPlan plan = bench::thumbnailsPausesScenario(set);
            const bench::ScenarioPlan pauses = bench::pausesScenario(set);
            QCOMPARE(plan.name, QStringLiteral("miniatures-pauses"));
            QCOMPARE(plan.generatedPages, bench::kThumbnailPages);
            QVERIFY(plan.measuresThumbnails);
            QVERIFY(plan.browsersOpen);
            QCOMPARE(plan.setupCommands.size(), std::size_t{1});
            QCOMPARE(plan.setupCommands[0].first, QStringLiteral("view.browsers"));
            QCOMPARE(plan.teardownCommands.size(), std::size_t{1});
            QVERIFY(plan.toolCommand.isEmpty());
            QVERIFY(!plan.freezeExempt);
            QCOMPARE(plan.maxCacheRefreshes, -1);
            QVERIFY(!plan.awaitFinalFold); // propre à « pauses » (onlyPausesAwaitsItsFinalFold)
            QCOMPARE(plan.inputs.size(), pauses.inputs.size());
            for (std::size_t i = 0; i < plan.inputs.size(); ++i) {
                QCOMPARE(plan.inputs[i].offsetNs, pauses.inputs[i].offsetNs);
                QCOMPARE(plan.inputs[i].pagePos, pauses.inputs[i].pagePos);
                QCOMPARE(plan.inputs[i].kind, pauses.inputs[i].kind);
            }
            // Chaque pause laisse le temps d'un rendu de miniature.
            std::int64_t lastRelease = -1;
            for (const auto& in : plan.inputs) {
                if (in.kind == InputKind::TabletPress && lastRelease >= 0) {
                    QVERIFY(in.offsetNs - lastRelease >= std::int64_t{ui::PageThumbnails::kSettleMs} * 1'000'000);
                }
                if (in.kind == InputKind::TabletRelease) {
                    lastRelease = in.offsetNs;
                }
            }
        }
    }

    // L4-T29 (amendement 5) : même scénario, mêmes pages, mais le panneau reste fermé (L4-T19 : aucun rendu).
    void closedPanelVariantKeepsItClosed()
    {
        for (const auto set : {bench::ScenarioSet::Complet, bench::ScenarioSet::Court}) {
            const bench::ScenarioPlan plan = bench::closedThumbnailsScenario(set);
            const bench::ScenarioPlan open = bench::thumbnailsScenario(set);
            QCOMPARE(plan.name, QStringLiteral("miniatures-ferme"));
            QCOMPARE(plan.generatedPages, open.generatedPages);
            QVERIFY(plan.measuresThumbnails);
            QVERIFY(!plan.browsersOpen);
            QVERIFY(plan.teardownCommands.empty());
            for (const auto& [command, argument] : plan.setupCommands) {
                Q_UNUSED(argument);
                QVERIFY2(command != QStringLiteral("view.browsers"), "le panneau doit rester fermé");
            }
            QCOMPARE(plan.inputs.size(), open.inputs.size());
            QVERIFY(!plan.freezeExempt);
            QCOMPARE(plan.maxCacheRefreshes, -1);
        }
        // Les autres scénarios ne relèvent aucune miniature.
        for (const auto& plan : bench::allScenarios(bench::ScenarioSet::Court)) {
            const bool thumbs = plan.name.startsWith(QStringLiteral("miniatures"));
            QCOMPARE(plan.measuresThumbnails, thumbs);
        }
    }

    // L3-C10 (S36, M3) : le plan de `gommer` et de `soulever` retire figements et reprises du verdict et pose
    // l'attendu des redessins ; aucun autre scénario n'est touché.
    void gesturePlansCarryTheirVerdictExemption()
    {
        for (const auto set : {bench::ScenarioSet::Complet, bench::ScenarioSet::Court}) {
            for (const bench::ScenarioPlan& plan : bench::allScenarios(set)) {
                const bool gesture = plan.toolCommand.size() > 0;
                QCOMPARE(plan.freezeExempt, gesture);
                QCOMPARE(plan.maxCacheRefreshes >= 0, gesture);
            }
            const bench::ScenarioPlan erase = bench::eraseScenario(set);
            QCOMPARE(erase.maxCacheRefreshes, erase.touchedStrokes + 1 + erase.pendingCut);
            QCOMPARE(bench::liftScenario(set).maxCacheRefreshes, 2);
        }
    }

    void eraseScenarioCrossesStrokes()
    {
        for (const auto set : {bench::ScenarioSet::Complet, bench::ScenarioSet::Court}) {
            const bench::ScenarioPlan plan = bench::eraseScenario(set);
            QCOMPARE(plan.name, QStringLiteral("gommer"));
            QCOMPARE(plan.toolCommand, QStringLiteral("tool.eraser"));
            QCOMPARE(plan.generatedPages, 1);
            QCOMPARE(plan.seedStrokes.size(), std::size_t{24});
            QCOMPARE(plan.pendingCut, 0);
            core::Page page = core::generatePage();
            core::ObjectId next = 1;
            for (const core::PageObject& o : page.objects) {
                next = std::max(next, o.common.id + 1);
            }
            const core::ObjectId firstSeed = next;
            for (const auto& points : plan.seedStrokes) {
                core::Stroke stroke;
                for (const QPointF& p : points) {
                    stroke.points.push_back({p.x(), p.y(), 0.5f});
                }
                core::PageObject object = std::move(stroke);
                object.common.layer = core::Layer::Top;
                object.common.id = next++;
                page.objects.push_back(std::move(object));
            }
            // Les traits générés ne sont pas des annotations : la gomme les saute.
            QVERIFY(!core::isAnnotation(page.objects.front()));
            QCOMPARE(plan.inputs.front().kind, InputKind::TabletPress);
            QCOMPARE(plan.inputs.back().kind, InputKind::TabletRelease);
            std::set<core::ObjectId> crossed;
            for (std::size_t i = 1; i < plan.inputs.size(); ++i) {
                const auto& a = plan.inputs[i - 1];
                const auto& b = plan.inputs[i];
                QCOMPARE(b.offsetNs - a.offsetNs, bench::kStylusPeriodNs);
                QVERIFY(!b.measured);
                QVERIFY(!core::hitsWitness({b.pagePos.x(), b.pagePos.y()}, bench::kEraserRadiusPage));
                const core::RectF box =
                    core::normalizedRect({a.pagePos.x(), a.pagePos.y()}, {b.pagePos.x(), b.pagePos.y()});
                const double r = bench::kEraserRadiusPage;
                const core::RectF reach{box.x - r, box.y - r, box.width + 2 * r, box.height + 2 * r};
                for (const core::ObjectId id : core::objectsTouching(page, reach)) {
                    if (id >= firstSeed) {
                        crossed.insert(id);
                    }
                }
            }
            QVERIFY2(crossed.size() >= 20, qPrintable(QString::number(crossed.size())));
            QCOMPARE(plan.touchedStrokes, static_cast<int>(crossed.size()));
        }
    }

    // L3-T19 (S36, M3) : un rectangle de sélection qui prend DEUX objets versés, puis un appui sur l'un d'eux
    // choisi par objectAt et un glissement de 300 px en 2 s.
    void liftScenarioDragsAnObject()
    {
        const bench::ScenarioPlan plan = bench::liftScenario(bench::ScenarioSet::Complet);
        QCOMPARE(plan.name, QStringLiteral("soulever"));
        QCOMPARE(plan.toolCommand, QStringLiteral("tool.select"));
        QCOMPARE(plan.generatedPages, 1);
        QVERIFY(plan.seedStrokes.empty());
        QVERIFY(plan.inputs.size() > 100);
        const core::Page page = core::generatePage();
        std::vector<std::size_t> presses;
        std::vector<std::size_t> releases;
        for (std::size_t i = 0; i < plan.inputs.size(); ++i) {
            if (plan.inputs[i].kind == InputKind::TabletPress) {
                presses.push_back(i);
            }
            if (plan.inputs[i].kind == InputKind::TabletRelease) {
                releases.push_back(i);
            }
        }
        QCOMPARE(presses.size(), std::size_t{2});
        QCOMPARE(releases.size(), std::size_t{2});
        // Rectangle : depuis un point libre, il prend exactement deux objets, hors témoins.
        const QPointF from = plan.inputs[presses[0]].pagePos;
        const QPointF to = plan.inputs[releases[0] - 1].pagePos;
        QVERIFY(!core::objectAt(page, {from.x(), from.y()}, core::kHitToleranceScreen));
        QVERIFY(from.x() < 0); // départ dans la marge de la vue
        const auto taken =
            core::objectsTouching(page, core::normalizedRect({from.x(), from.y()}, {to.x(), to.y()}));
        // La page générée est dense (emprises qui se recouvrent) : aucun rectangle ne prend exactement deux
        // objets ; le plan en prend le moins possible (plusieurs : le cas de M3).
        QVERIFY2(taken.size() >= 2 && taken.size() <= 8, qPrintable(QString::number(taken.size())));
        QCOMPARE(plan.liftedObjects, static_cast<int>(taken.size()));
        QCOMPARE(plan.liftedObjects, static_cast<int>(taken.size()));
        for (const core::ObjectId id : taken) {
            QVERIFY(id < page.objects[page.objects.size() - 3].common.id);
        }
        // Appui sur l'un des deux, puis 300 px en 2 s.
        const QPointF grab = plan.inputs[presses[1]].pagePos;
        const auto hit = core::objectAt(page, {grab.x(), grab.y()}, core::kHitToleranceScreen);
        QVERIFY(hit.has_value());
        QVERIFY(std::find(taken.begin(), taken.end(), *hit) != taken.end());
        const QPointF end = plan.inputs[releases[1] - 1].pagePos;
        QVERIFY(std::abs(std::hypot(end.x() - grab.x(), end.y() - grab.y()) - 300.0) < 1.0);
        QCOMPARE(plan.inputs[releases[1] - 1].offsetNs - plan.inputs[presses[1]].offsetNs,
                 std::int64_t{2'000'000'000});
        QVERIFY(end.x() >= 220.0 && end.x() <= 804.0);
    }

    // L4-T14 (S7.5, V9) : « forme sur page annotée ». Page générée dont la couche du dessus porte des
    // annotations ; k >= 10 rectangles posés un par un par glissé (préréglage choisi par commande avant la
    // mesure, outil « tool.shape ») : chaque insertion est hors fin de page et redessine le cache une fois.
    void shapeOnAnnotatedPageScenario()
    {
        for (const auto set : {bench::ScenarioSet::Complet, bench::ScenarioSet::Court}) {
            const bench::ScenarioPlan plan = bench::shapeScenario(set);
            QCOMPARE(plan.name, QStringLiteral("forme-page-annotee"));
            QCOMPARE(plan.generatedPages, 1);
            QCOMPARE(plan.toolCommand, QStringLiteral("tool.shape"));
            QVERIFY(!plan.seedStrokes.empty()); // annotations : la couche du dessus n'est pas vide
            QVERIFY(plan.measuresShapePreview);
            QVERIFY(plan.freezeExempt);
            QVERIFY(plan.placedShapes >= 10);
            QCOMPARE(plan.maxCacheRefreshes, plan.placedShapes); // cache.refreshes = k
            QCOMPARE(plan.setupCommands.size(), std::size_t{1});
            QCOMPARE(plan.setupCommands[0].first, QStringLiteral("shape.choose"));
            QCOMPARE(plan.setupCommands[0].second.toString(), QStringLiteral("rectangle"));
            std::vector<std::size_t> presses;
            std::vector<std::size_t> releases;
            for (std::size_t i = 0; i < plan.inputs.size(); ++i) {
                const auto& in = plan.inputs[i];
                QVERIFY(in.kind != InputKind::Command && in.kind != InputKind::Wheel);
                QVERIFY(!in.measured); // aucune latence d'encre : on mesure l'aperçu, pas un trait
                QVERIFY(!core::hitsWitness({in.pagePos.x(), in.pagePos.y()}, 16.0));
                QVERIFY(in.pagePos.x() >= 220.0 && in.pagePos.x() <= 804.0);
                QVERIFY(in.pagePos.y() >= 150.0 && in.pagePos.y() <= 618.0);
                if (i > 0) {
                    QVERIFY(in.offsetNs >= plan.inputs[i - 1].offsetNs);
                }
                if (in.kind == InputKind::TabletPress) {
                    presses.push_back(i);
                }
                if (in.kind == InputKind::TabletRelease) {
                    releases.push_back(i);
                }
            }
            QCOMPARE(presses.size(), static_cast<std::size_t>(plan.placedShapes));
            QCOMPARE(releases.size(), presses.size());
            for (std::size_t k = 0; k < presses.size(); ++k) {
                const QPointF a = plan.inputs[presses[k]].pagePos;
                const QPointF b = plan.inputs[releases[k]].pagePos;
                QVERIFY(releases[k] > presses[k] + 1);
                QVERIFY(std::abs(b.x() - a.x()) > 3.0 &&
                        std::abs(b.y() - a.y()) > 3.0); // un glissé, pas un clic
                if (k > 0) { // le redessin d'une insertion se termine avant l'appui suivant
                    QVERIFY(plan.inputs[presses[k]].offsetNs - plan.inputs[releases[k - 1]].offsetNs >=
                            500'000'000);
                }
            }
            const auto windows = bench::dragWindows(plan);
            QCOMPARE(windows.size(), presses.size());
            QCOMPARE(windows[0].first, plan.inputs[presses[0]].offsetNs);
            QCOMPARE(windows[0].second, plan.inputs[releases[0]].offsetNs);
        }
    }

    // L4-T14 : trames comptées dans les glissés seulement ; l'intervalle d'une trame est celui qui la
    // précède. Fenêtres fermées aux deux bouts.
    void shapePreviewCountsFramesInsideDrags()
    {
        const std::vector<std::int64_t> swaps{0, 10'000'000, 20'000'000, 30'000'000, 80'000'000, 90'000'000};
        const auto one = bench::shapePreviewStats(swaps, {{15'000'000, 35'000'000}});
        QCOMPARE(one.frames, 2);
        QCOMPARE(one.maxMs, 10.0);
        QCOMPARE(one.p95Ms, 10.0);
        const auto two =
            bench::shapePreviewStats(swaps, {{15'000'000, 35'000'000}, {75'000'000, 95'000'000}});
        QCOMPARE(two.frames, 4);
        QCOMPARE(two.maxMs, 50.0);
        QVERIFY(two.p95Ms >= 10.0 && two.p95Ms <= 50.0);
        const auto none = bench::shapePreviewStats(swaps, {{31'000'000, 79'000'000}});
        QCOMPARE(none.frames, 0);
        QCOMPARE(none.maxMs, 0.0);
    }

    // L4-T14 (S7.5) : page générée dont la grille est dessinée au-dessus des objets ; les traits du scénario
    // « stroke », que la grille du dessus ne fait plus redessiner (L4-T7) : aucun redessin entre deux
    // versements de la minuterie, et ses traits sont à 300 ms les uns des autres, sous le délai.
    void gridAboveScenarioDrawsStrokes()
    {
        const core::Grid grid = bench::gridAboveFixture();
        QVERIFY(grid.visible && grid.drawnAbove && !grid.levels.empty() && !grid.snapEnabled);
        for (const auto set : {bench::ScenarioSet::Complet, bench::ScenarioSet::Court}) {
            const bench::ScenarioPlan plan = bench::gridAboveScenario(set);
            const bench::ScenarioPlan stroke = bench::strokeScenario(set);
            QCOMPARE(plan.name, QStringLiteral("grille-dessus"));
            QCOMPARE(plan.generatedPages, 1);
            QVERIFY(plan.gridAbove);
            QVERIFY(plan.toolCommand.isEmpty()); // le stylo : les traits de « stroke »
            QVERIFY(plan.setupCommands.empty());
            QVERIFY(!plan.freezeExempt); // jugé comme « stroke » (figements compris)
            QCOMPARE(plan.maxCacheRefreshes, -1);
            QVERIFY(!plan.measuresShapePreview);
            QCOMPARE(plan.inputs.size(), stroke.inputs.size());
            for (std::size_t i = 0; i < plan.inputs.size(); ++i) {
                QCOMPARE(plan.inputs[i].offsetNs, stroke.inputs[i].offsetNs);
                QCOMPARE(plan.inputs[i].pagePos, stroke.inputs[i].pagePos);
                QCOMPARE(plan.inputs[i].kind, stroke.inputs[i].kind);
                QCOMPARE(plan.inputs[i].measured, stroke.inputs[i].measured);
            }
            QVERIFY(std::any_of(plan.inputs.begin(), plan.inputs.end(),
                                [](const bench::PlannedInput& in) { return in.measured; }));
        }
    }

    // L10-T1 (DA11) : chaque reprise tombe après le délai d'inactivité du cache, décalée de 0 à 64 ms (court
    // : 0 à 48), pour tomber dans la fenêtre du redessin différé ; page du banc rechargée (300 objets).
    void pausesResumeInsideTheFoldWindow()
    {
        for (const auto set : {bench::ScenarioSet::Complet, bench::ScenarioSet::Court}) {
            const bench::ScenarioPlan plan = bench::pausesScenario(set);
            QCOMPARE(plan.name, QStringLiteral("pauses"));
            QCOMPARE(plan.generatedPages, 1);
            QVERIFY(!plan.pageChecks); // page rechargée, mais aucun relevé du scénario « page »
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

    // L3-C14 : seul « pauses » ferme sa mesure sur le versement différé de son dernier trait ; « stroke »
    // garde ses traits en attente (aucun redessin du cache pendant sa mesure, tst_bench_result).
    void onlyPausesAwaitsItsFinalFold()
    {
        for (const bench::ScenarioSet set : {bench::ScenarioSet::Court, bench::ScenarioSet::Complet}) {
            for (const bench::ScenarioPlan& plan : bench::allScenarios(set)) {
                QCOMPARE(plan.awaitFinalFold, plan.name == QStringLiteral("pauses"));
            }
        }
    }

    // L2-T23 : changements de page à intervalle fixe par commandes nommées (aucun événement posté, aucune
    // encre), puis deux zooms posés sur la page peinte ; document de deux pages générées chargé avant.
    void pageScenarioFlipsPages()
    {
        const bench::ScenarioPlan court = bench::pageScenario(bench::ScenarioSet::Court);
        QCOMPARE(court.name, QStringLiteral("page"));
        QCOMPARE(court.generatedPages, 2);
        QVERIFY(court.pageChecks); // relevés propres au scénario (runner.cpp), lus par ce booléen
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
        QVERIFY(complet.pageChecks);
        QCOMPARE(complet.inputs[19].command,
                 QStringLiteral("page.previous"));                         // 20 changements : retour page 1
        QCOMPARE(complet.inputs[20].command, QStringLiteral("page.next")); // puis la page peinte
        QCOMPARE(complet.inputs.back().command, QStringLiteral("view.fit"));
        // Les scénarios de référence gardent la page en place.
        for (const auto& plan : {bench::strokeScenario(bench::ScenarioSet::Court),
                                 bench::zoomScenario(bench::ScenarioSet::Court),
                                 bench::scrollScenario(bench::ScenarioSet::Court)}) {
            QCOMPARE(plan.generatedPages, 0);
            QVERIFY(!plan.pageChecks);
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
