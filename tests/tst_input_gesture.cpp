#include <actcraft/input/gesture.h>

#include <QTest>

using namespace actcraft::input;

class TstInputGesture : public QObject
{
    Q_OBJECT

private slots:
    void pinchScalesAroundCenter()
    {
        TwoFingerGesture g;
        g.begin({400, 300}, {600, 300});
        const GestureStep step = g.update({300, 300}, {700, 300});
        QCOMPARE(step.scale, 2.0);
        QCOMPARE(step.anchor.x, 500.0);
        QCOMPARE(step.center.x, 500.0);
    }
    void panFollowsCenter()
    {
        TwoFingerGesture g;
        g.begin({400, 300}, {600, 300});
        const GestureStep step = g.update({450, 350}, {650, 350});
        QCOMPARE(step.scale, 1.0);
        QCOMPARE(step.center.x, 550.0);
        QCOMPARE(step.center.y, 350.0);
    }
    void deadZoneKeepsZoom()
    {
        TwoFingerGesture g;
        g.begin({400, 300}, {600, 300});
        QCOMPARE(g.update({398, 300}, {603, 300}).scale, 1.0); // 2,5 % < kPinchDeadZone
    }
    // Juste au-delà de la zone morte : le zoom suit l'écart, sans saut (pas de valeur ramenée au seuil).
    void pinchOutsideDeadZoneZooms()
    {
        TwoFingerGesture g;
        g.begin({400, 300}, {600, 300});
        QCOMPARE(g.update({397, 300}, {604, 300}).scale, 1.035); // 3,5 % > kPinchDeadZone
    }
    // Le point d'ancrage reste le milieu à l'appui, quand les doigts se sont déplacés depuis.
    void anchorStaysWhereTheGestureBegan()
    {
        TwoFingerGesture g;
        g.begin({100, 200}, {300, 400});
        const GestureStep step = g.update({500, 600}, {700, 800});
        QCOMPARE(step.anchor.x, 200.0);
        QCOMPARE(step.anchor.y, 300.0);
        QCOMPARE(step.center.x, 600.0);
        QCOMPARE(step.center.y, 700.0);
    }
    // Sans begin : échelle 1, ancre et centre au milieu des doigts.
    void updateWithoutBeginIsNeutral()
    {
        TwoFingerGesture g;
        const GestureStep step = g.update({0, 0}, {40, 20});
        QCOMPARE(step.scale, 1.0);
        QCOMPARE(step.center.x, 20.0);
        QCOMPARE(step.center.y, 10.0);
        QCOMPARE(step.anchor.x, 20.0);
        QCOMPARE(step.anchor.y, 10.0);
    }
    void endDeactivates()
    {
        TwoFingerGesture g;
        g.begin({0, 0}, {10, 0});
        QVERIFY(g.active());
        g.end();
        QVERIFY(!g.active());
    }
};

QTEST_GUILESS_MAIN(TstInputGesture)
#include "tst_input_gesture.moc"
