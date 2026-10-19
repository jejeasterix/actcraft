#include <actcraft/input/routing.h>

#include <QTest>

#include <memory>

using namespace actcraft::input;

namespace {

Sample at(DeviceKind device, Phase phase, double x, std::uint64_t pointer = 0)
{
    Sample out;
    out.device = device;
    out.phase = phase;
    out.x = x;
    out.y = 50;
    out.pointer = pointer;
    return out;
}

// Couverture d'essai : la bande x < 100.
bool coveredLeftBand(const Sample& press)
{
    return press.x < 100;
}

// Politique qui compte ses appels et retient l'état vu : une couverture se décide avant elle.
class CountingPolicy final : public RoutingPolicy
{
public:
    explicit CountingPolicy(int& calls, RouterState& seen) : m_calls(calls), m_seen(seen) {}
    Decision decide(const Sample& press, const RouterState& state) const override
    {
        ++m_calls;
        m_seen = state;
        Decision decision;
        decision.route = Route::Tool;
        // Deuxième doigt : le premier passe au geste (mécanisme du routeur, politique du lot 3 en L3-T5).
        if (press.device == DeviceKind::Touch && state.toolTouches == 1) {
            decision.route = Route::Gesture;
            decision.endToolContacts = true;
        }
        return decision;
    }

private:
    int& m_calls;
    RouterState& m_seen;
};

} // namespace

class TstInputRouting : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L0-S : rempli par L3-T5."); }

    // D-S1 : une décision Cover est retenue pour tout le contact (le point sort de la couverture : toujours
    // Cover), puis rendue au relâché et oubliée.
    void coverRouteHeldForWholeContact()
    {
        PointerRouter router;
        router.setCoverTest(coveredLeftBand);
        QCOMPARE(router.route(at(DeviceKind::Stylus, Phase::Press, 50)).route, Route::Cover);
        QCOMPARE(router.route(at(DeviceKind::Stylus, Phase::Move, 500)).route, Route::Cover);
        QCOMPARE(router.route(at(DeviceKind::Stylus, Phase::Release, 500)).route, Route::Cover);
        QCOMPARE(router.route(at(DeviceKind::Stylus, Phase::Move, 500)).route, Route::Ignore);
    }

    // Bloquant 1 de la relecture : un appui couvert du stylet, jamais relâché auprès du routeur, ne pose pas
    // stylusDown ; un doigt non couvert va à l'outil.
    void coverLeavesTheStateAlone()
    {
        PointerRouter router;
        router.setCoverTest(coveredLeftBand);
        QCOMPARE(router.route(at(DeviceKind::Stylus, Phase::Press, 50)).route, Route::Cover);
        QCOMPARE(router.route(at(DeviceKind::Touch, Phase::Press, 500, 3)).route, Route::Tool);
    }

    // La couverture est consultée avant la politique : un appui couvert ne l'appelle pas, et l'état qu'elle
    // voit ensuite ne compte pas le contact couvert.
    void coverIsDecidedBeforeThePolicy()
    {
        int calls = 0;
        RouterState seen;
        PointerRouter router;
        router.setPolicy(std::make_unique<CountingPolicy>(calls, seen));
        router.setCoverTest(coveredLeftBand);
        QCOMPARE(router.route(at(DeviceKind::Stylus, Phase::Press, 50)).route, Route::Cover);
        QCOMPARE(calls, 0);
        QCOMPARE(router.route(at(DeviceKind::Touch, Phase::Press, 500, 3)).route, Route::Tool);
        QCOMPARE(calls, 1);
        QVERIFY(!seen.stylusDown);
        QCOMPARE(seen.toolTouches, 0);
        QCOMPARE(seen.gestureTouches, 0);
    }

    // Un nouvel appui du même pointeur (relâché jamais vu) redécide.
    void pressRedecides()
    {
        PointerRouter router;
        router.setCoverTest(coveredLeftBand);
        QCOMPARE(router.route(at(DeviceKind::Stylus, Phase::Press, 50)).route, Route::Cover);
        QCOMPARE(router.route(at(DeviceKind::Stylus, Phase::Press, 500)).route, Route::Tool);
        QCOMPARE(router.route(at(DeviceKind::Stylus, Phase::Move, 50)).route, Route::Tool);
    }

    // Politique minimale du socle : stylet, souris, doigt non couverts → outil, utilisateur 1.
    void uncoveredGoesToTheTool()
    {
        PointerRouter router;
        router.setCoverTest(coveredLeftBand);
        const Decision stylus = router.route(at(DeviceKind::Stylus, Phase::Press, 500));
        QCOMPARE(stylus.route, Route::Tool);
        QCOMPARE(stylus.user, kFirstUser);
        QCOMPARE(router.route(at(DeviceKind::Stylus, Phase::Release, 500)).route, Route::Tool);
        QCOMPARE(router.route(at(DeviceKind::Mouse, Phase::Press, 500)).route, Route::Tool);
        QCOMPARE(router.route(at(DeviceKind::Mouse, Phase::Release, 500)).route, Route::Tool);
        QCOMPARE(router.route(at(DeviceKind::Touch, Phase::Press, 500, 1)).route, Route::Tool);
        QCOMPARE(router.route(at(DeviceKind::Touch, Phase::Move, 600, 1)).route, Route::Tool);
    }

    // Paume : un doigt pendant un contact du stylet routé vers l'outil ne va nulle part ; relâché, plus rien
    // ne bloque.
    void fingerIgnoredWhileStylusToolContact()
    {
        PointerRouter router;
        QCOMPARE(router.route(at(DeviceKind::Stylus, Phase::Press, 500)).route, Route::Tool);
        QCOMPARE(router.route(at(DeviceKind::Touch, Phase::Press, 600, 2)).route, Route::Ignore);
        router.release(2); // la vue rend aussitôt un contact qu'elle ne prend pas
        QCOMPARE(router.route(at(DeviceKind::Stylus, Phase::Release, 500)).route, Route::Tool);
        QCOMPARE(router.route(at(DeviceKind::Touch, Phase::Press, 600, 2)).route, Route::Tool);
    }

    // Contact rendu par la vue (deuxième doigt, annulation, appui non pris) : oublié.
    void releaseForgetsAContact()
    {
        PointerRouter router;
        QCOMPARE(router.route(at(DeviceKind::Touch, Phase::Press, 500, 3)).route, Route::Tool);
        router.release(3);
        QCOMPARE(router.route(at(DeviceKind::Touch, Phase::Move, 510, 3)).route, Route::Ignore);
        router.release(3); // inconnu : sans effet
        QCOMPARE(router.route(at(DeviceKind::Touch, Phase::Release, 510, 3)).route, Route::Ignore);
    }

    // Mécanisme d'en-tête : endToolContacts fait passer les doigts retenus vers l'outil au geste, et
    // gestureTouches les compte jusqu'à leur relâché.
    void endToolContactsMovesFingersToTheGesture()
    {
        int calls = 0;
        RouterState seen;
        PointerRouter router;
        router.setPolicy(std::make_unique<CountingPolicy>(calls, seen));
        QCOMPARE(router.route(at(DeviceKind::Touch, Phase::Press, 500, 1)).route, Route::Tool);
        const Decision second = router.route(at(DeviceKind::Touch, Phase::Press, 700, 2));
        QCOMPARE(second.route, Route::Gesture);
        QVERIFY(second.endToolContacts);
        QCOMPARE(router.route(at(DeviceKind::Touch, Phase::Move, 510, 1)).route, Route::Gesture);
        QCOMPARE(router.gestureTouches(), 2);
        router.route(at(DeviceKind::Touch, Phase::Release, 510, 1));
        QCOMPARE(router.gestureTouches(), 1);
        router.reset();
        QCOMPARE(router.gestureTouches(), 0);
        QCOMPARE(router.route(at(DeviceKind::Touch, Phase::Move, 700, 2)).route, Route::Ignore);
    }
};

QTEST_GUILESS_MAIN(TstInputRouting)
#include "tst_input_routing.moc"
