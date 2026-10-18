#include <actcraft/features/pagezoom.h>

#include <QtTest>

#include <cmath>

using namespace actcraft::features;

class TstFeaturesPageZoom : public QObject
{
    Q_OBJECT

private slots:
    void holdZoomsInAroundThePoint()
    {
        PageZoom z(1.0); // zoom de départ
        z.press(QPointF(300, 200), PageZoom::Direction::In, 0);
        QCOMPARE(z.zoomAt(500'000'000), 1.0 * std::pow(PageZoom::kRatePerSecond, 0.5)); // 500 ms
        QCOMPARE(z.anchor(), QPointF(300, 200));
    }
    void rightButtonZoomsOut()
    {
        PageZoom z(2.0);
        z.press(QPointF(0, 0), PageZoom::Direction::Out, 0);
        QVERIFY(z.zoomAt(1'000'000'000) < 2.0);
    }
    void clampedToViewLimits()
    {
        PageZoom z(1.0);
        z.press(QPointF(0, 0), PageZoom::Direction::In, 0);
        QCOMPARE(z.zoomAt(60'000'000'000), PageZoom::kMaxZoom);
        PageZoom out(1.0);
        out.press(QPointF(0, 0), PageZoom::Direction::Out, 0);
        QCOMPARE(out.zoomAt(60'000'000'000), PageZoom::kMinZoom); // borne basse
        QCOMPARE(PageZoom(10.0).zoomAt(0), PageZoom::kMaxZoom);   // hors appui aussi
        QCOMPARE(PageZoom(0.01).zoomAt(0), PageZoom::kMinZoom);
    }
    void dragTurnsIntoPan()
    {
        PageZoom z(1.0);
        z.press(QPointF(100, 100), PageZoom::Direction::In, 0);
        QVERIFY(z.move(QPointF(100 + PageZoom::kPanThreshold + 1, 100), 50'000'000)); // vrai : panoramique
        QVERIFY(z.panning());
        QCOMPARE(z.zoomAt(500'000'000), 1.0); // plus de zoom une fois en panoramique
        z.release(600'000'000);
        QVERIFY(!z.panning()); // le relâché termine le panoramique
        QVERIFY(!z.pressed());

        // sous le seuil (et au seuil exact) : pas de panoramique, le zoom continue
        PageZoom small(1.0);
        small.press(QPointF(100, 100), PageZoom::Direction::In, 0);
        QVERIFY(!small.move(QPointF(100 + PageZoom::kPanThreshold - 1, 100), 10'000'000));
        QVERIFY(!small.move(QPointF(100 + PageZoom::kPanThreshold, 100), 20'000'000));
        QVERIFY(!small.panning());
        QVERIFY(small.zoomAt(500'000'000) > 1.0);
    }
    void doubleClickFits()
    {
        PageZoom z(3.0);
        QVERIFY(z.isDoubleClick(QPointF(10, 10), 0) == false);
        QVERIFY(z.isDoubleClick(QPointF(12, 11), 200'000'000));

        PageZoom late(3.0); // second appui trop tardif
        QVERIFY(!late.isDoubleClick(QPointF(10, 10), 0));
        QVERIFY(!late.isDoubleClick(QPointF(10, 10), PageZoom::kDoubleClickNs + 1));
        PageZoom far(3.0); // second appui trop loin
        QVERIFY(!far.isDoubleClick(QPointF(10, 10), 0));
        QVERIFY(!far.isDoubleClick(QPointF(10 + PageZoom::kDoubleClickPixels + 1, 10), 100'000'000));
        PageZoom edge(3.0); // aux bornes exactes : encore un double-clic
        QVERIFY(!edge.isDoubleClick(QPointF(10, 10), 0));
        QVERIFY(edge.isDoubleClick(QPointF(10 + PageZoom::kDoubleClickPixels, 10), PageZoom::kDoubleClickNs));
    }
    void releaseFreezesTheZoom()
    {
        PageZoom z(1.0);
        z.press(QPointF(10, 10), PageZoom::Direction::In, 0);
        z.release(1'000'000'000);
        QCOMPARE(z.zoomAt(3'000'000'000), 2.0); // figé au relâché : 2 fois en une seconde
        QVERIFY(!z.pressed());
    }
    void panDeltaIsConsumed()
    {
        PageZoom z(1.0);
        z.press(QPointF(100, 100), PageZoom::Direction::In, 0);
        QVERIFY(z.move(QPointF(120, 100), 10'000'000));
        QVERIFY(z.move(QPointF(130, 105), 20'000'000));
        QCOMPARE(z.panDelta(), QPointF(30, 5));
        QCOMPARE(z.panDelta(), QPointF(0, 0));

        // le déplacement fait sous le seuil n'est pas perdu : le premier delta couvre tout le chemin
        PageZoom p(1.0);
        p.press(QPointF(100, 100), PageZoom::Direction::In, 0);
        QVERIFY(!p.move(QPointF(105, 100), 5'000'000));
        QVERIFY(p.move(QPointF(120, 100), 10'000'000));
        QCOMPARE(p.panDelta(), QPointF(20, 0));
    }
    void tripleClickCountsOnlyOnce()
    {
        PageZoom z(1.0);
        QVERIFY(!z.isDoubleClick(QPointF(0, 0), 0));
        QVERIFY(z.isDoubleClick(QPointF(0, 0), 100'000'000));
        QVERIFY(!z.isDoubleClick(QPointF(0, 0), 200'000'000)); // le troisième appui recommence
    }
};

QTEST_GUILESS_MAIN(TstFeaturesPageZoom)
#include "tst_features_pagezoom.moc"
