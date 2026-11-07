#include <actcraft/input/touchmodes.h>

#include <QtTest>

using namespace actcraft::input;

namespace {
Sample press(DeviceKind kind, std::uint64_t pointer)
{
    Sample s;
    s.device = kind;
    s.pointer = pointer;
    s.phase = Phase::Press;
    s.x = 10;
    s.y = 10;
    return s;
}

Sample moveOf(Sample s)
{
    s.phase = Phase::Move;
    return s;
}
} // namespace

class TstInputTouchModes : public QObject
{
    Q_OBJECT
private slots:
    void ignoreModeDropsFingers()
    {
        PointerRouter router;
        router.setPolicy(makeTouchPolicy(TouchMode::Ignore));
        QCOMPARE(router.route(press(DeviceKind::Touch, 7)).route, Route::Ignore);
        QCOMPARE(router.route(press(DeviceKind::Stylus, 0)).route, Route::Tool);
    }
    void penModeFingerDrawsWithPen()
    {
        PointerRouter router;
        router.setPolicy(makeTouchPolicy(TouchMode::Pen));
        const Decision d = router.route(press(DeviceKind::Touch, 7));
        QCOMPARE(d.route, Route::Tool);
        QCOMPARE(d.fingerTool, FingerTool::Pen);
        // L'outil imposé est retenu avec la décision : chaque mouvement du doigt le porte (L11-T5).
        QCOMPARE(router.route(moveOf(press(DeviceKind::Touch, 7))).fingerTool, FingerTool::Pen);
        // Le stylet garde l'outil courant et prend la main au doigt qui écrit (M4 b, comme en mode full).
        const Decision stylus = router.route(press(DeviceKind::Stylus, 0));
        QCOMPARE(stylus.route, Route::Tool);
        QCOMPARE(stylus.fingerTool, FingerTool::Current);
        QVERIFY(stylus.endToolContacts);
        QCOMPARE(router.route(moveOf(press(DeviceKind::Touch, 7))).route, Route::Ignore);
    }
    void selectModeFingerSelects()
    {
        PointerRouter router;
        router.setPolicy(makeTouchPolicy(TouchMode::Select));
        const Decision d = router.route(press(DeviceKind::Touch, 7));
        QCOMPARE(d.route, Route::Tool);
        QCOMPARE(d.fingerTool, FingerTool::Select);
        QCOMPARE(router.route(press(DeviceKind::Mouse, 0)).fingerTool, FingerTool::Current);
    }
    void fullModeIsTheLot3Behaviour()
    {
        for (bool explicitFull : {false, true}) {
            PointerRouter router; // FullTouchPolicy par défaut
            if (explicitFull) {
                router.setPolicy(makeTouchPolicy(TouchMode::Full));
            }
            const Decision first = router.route(press(DeviceKind::Touch, 7));
            QCOMPARE(first.route, Route::Tool);
            QCOMPARE(first.fingerTool, FingerTool::Current); // outil courant de l'utilisateur (H11-15)
            const Decision second = router.route(press(DeviceKind::Touch, 8));
            QCOMPARE(second.route, Route::Gesture);
            QVERIFY(second.endToolContacts);
            QCOMPARE(router.gestureTouches(), 2);
        }
    }
    void noGestureOutsideFullMode()
    {
        for (TouchMode mode : {TouchMode::Pen, TouchMode::Select}) {
            PointerRouter router;
            router.setPolicy(makeTouchPolicy(mode));
            router.route(press(DeviceKind::Touch, 7));
            QCOMPARE(router.route(press(DeviceKind::Touch, 8)).route, Route::Ignore);
            QCOMPARE(router.gestureTouches(), 0);
        }
        // Deux doigts du même événement : le premier prend l'outil imposé, le second rien (pas de geste).
        for (TouchMode mode : {TouchMode::Pen, TouchMode::Select}) {
            PointerRouter router;
            router.setPolicy(makeTouchPolicy(mode));
            const Sample both[] = {press(DeviceKind::Touch, 7), press(DeviceKind::Touch, 8)};
            const auto decisions = router.routePresses(both);
            QCOMPARE(decisions.size(), std::size_t(2));
            QCOMPARE(decisions[0].route, Route::Tool);
            QCOMPARE(decisions[1].route, Route::Ignore);
            QCOMPARE(router.gestureTouches(), 0);
        }
    }
    void palmIgnoredInEveryMode()
    {
        for (TouchMode mode : {TouchMode::Ignore, TouchMode::Pen, TouchMode::Select, TouchMode::Full}) {
            PointerRouter router;
            router.setPolicy(makeTouchPolicy(mode));
            router.route(press(DeviceKind::Stylus, 0));
            QCOMPARE(router.route(press(DeviceKind::Touch, 7)).route, Route::Ignore);
            // Une paume pose souvent plusieurs points dans le même événement : aucun geste pour autant
            // (D11-3).
            const Sample palm[] = {press(DeviceKind::Touch, 8), press(DeviceKind::Touch, 9)};
            for (const Decision& d : router.routePresses(palm)) {
                QCOMPARE(d.route, Route::Ignore);
            }
            QCOMPARE(router.gestureTouches(), 0);
        }
    }
    void sameEventTwoFingersGoToGesture()
    {
        PointerRouter router;
        const Sample both[] = {press(DeviceKind::Touch, 7), press(DeviceKind::Touch, 8)};
        const auto decisions = router.routePresses(both);
        QCOMPARE(decisions.size(), std::size_t(2));
        // Décidé seul, le premier doigt irait à l'outil et y verserait son point (H3-16).
        QCOMPARE(decisions[0].route, Route::Gesture);
        QCOMPARE(decisions[1].route, Route::Gesture);
        QCOMPARE(router.gestureTouches(), 2);

        // Au-delà de deux doigts, rien, comme au lot 3.
        PointerRouter three;
        const Sample fingers[] = {press(DeviceKind::Touch, 7), press(DeviceKind::Touch, 8),
                                  press(DeviceKind::Touch, 9)};
        const auto threeDecisions = three.routePresses(fingers);
        QCOMPARE(threeDecisions.size(), std::size_t(3));
        QCOMPARE(threeDecisions[0].route, Route::Gesture);
        QCOMPARE(threeDecisions[1].route, Route::Gesture);
        QCOMPARE(threeDecisions[2].route, Route::Ignore);
        QCOMPARE(three.gestureTouches(), 2);

        // Un doigt posé sur une surcouche ne compte pas : l'autre est seul sur la page et va à l'outil.
        PointerRouter covered;
        covered.setCoverTest([](const Sample& s) { return s.pointer == 8; });
        const auto coveredDecisions = covered.routePresses(both);
        QCOMPARE(coveredDecisions.size(), std::size_t(2));
        QCOMPARE(coveredDecisions[0].route, Route::Tool);
        QCOMPARE(coveredDecisions[1].route, Route::Cover);
        QCOMPARE(covered.gestureTouches(), 0);
    }
    void idsRoundTrip()
    {
        QVERIFY(touchModeId(TouchMode::Ignore) == "ignore");
        QVERIFY(touchModeId(TouchMode::Pen) == "pen");
        QVERIFY(touchModeId(TouchMode::Select) == "select");
        QVERIFY(touchModeId(TouchMode::Full) == "full");
        for (TouchMode mode : {TouchMode::Ignore, TouchMode::Pen, TouchMode::Select, TouchMode::Full}) {
            TouchMode back = TouchMode::Full;
            QVERIFY(touchModeFromId(touchModeId(mode), back));
            QCOMPARE(back, mode);
        }
        TouchMode unknown = TouchMode::Pen;
        QVERIFY(!touchModeFromId("zzz", unknown));
        QCOMPARE(unknown, TouchMode::Pen);
    }
    void policyChangeKeepsContacts()
    {
        PointerRouter router;
        router.route(press(DeviceKind::Touch, 7)); // Tool, mode full
        router.setPolicy(makeTouchPolicy(TouchMode::Ignore));
        Sample move = press(DeviceKind::Touch, 7);
        move.phase = Phase::Move;
        QCOMPARE(router.route(move).route, Route::Tool); // décision retenue jusqu'au relâché
    }
};

QTEST_GUILESS_MAIN(TstInputTouchModes)
#include "tst_input_touchmodes.moc"
