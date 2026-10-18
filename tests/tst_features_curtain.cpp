#include <actcraft/features/curtain.h>

#include <QtTest>

using namespace actcraft::features;

class TstFeaturesCurtain : public QObject
{
    Q_OBJECT
private slots:
    void closedCoversEverything()
    {
        Curtain c(QSizeF(1000, 800));
        QCOMPARE(c.coverage(), 1.0);
        QCOMPARE(c.covered(), QRectF(0, 0, 1000, 800));
    }
    void dragFromTopUncovers()
    {
        Curtain c(QSizeF(1000, 800));
        QVERIFY(c.press(QPointF(500, 3))); // à moins de kGrabMargin du bord haut
        c.move(QPointF(500, 400));
        c.release();
        QCOMPARE(c.covered(), QRectF(0, 400, 1000, 400));
        QCOMPARE(c.coverage(), 0.5);
    }
    void dragFromLeftUncovers()
    {
        Curtain c(QSizeF(1000, 800));
        QVERIFY(c.press(QPointF(2, 300)));
        c.move(QPointF(250, 300));
        QCOMPARE(c.covered(), QRectF(250, 0, 750, 800));
    }
    void pressInsideDoesNotGrab()
    {
        Curtain c(QSizeF(1000, 800));
        QVERIFY(!c.press(QPointF(500, 400)));
    }
    void edgeOfAnOpenCurtainCanBeDraggedBack()
    {
        Curtain c(QSizeF(1000, 800));
        c.press(QPointF(500, 1));
        c.move(QPointF(500, 400));
        c.release();
        QVERIFY(c.press(QPointF(500, 402))); // le bord du store est à 400
        c.move(QPointF(500, 100));
        QCOMPARE(c.covered().top(), 100.0);
    }
    void clampedToTheArea()
    {
        Curtain c(QSizeF(1000, 800));
        c.press(QPointF(500, 1));
        c.move(QPointF(500, 5000));
        QCOMPARE(c.coverage(), 0.0);
    }
    void resizeKeepsProportions()
    {
        Curtain c(QSizeF(1000, 800));
        c.press(QPointF(500, 1));
        c.move(QPointF(500, 400));
        c.release();
        c.setArea(QSizeF(500, 400));
        QCOMPARE(c.covered(), QRectF(0, 200, 500, 200));
    }
    void startModeAndDistance()
    {
        Curtain c(QSizeF(1000, 800));
        c.reset(Curtain::Start::Top, 100); // F-592 : mode « supérieur », distance 100
        QCOMPARE(c.covered(), QRectF(0, 100, 1000, 700));
    }
    void releaseEndsTheDrag()
    {
        Curtain c(QSizeF(1000, 800));
        QVERIFY(c.press(QPointF(500, 2)));
        QVERIFY(c.dragging());
        c.release();
        QVERIFY(!c.dragging());
        c.move(QPointF(500, 600)); // sans saisie : rien ne bouge
        QCOMPARE(c.coverage(), 1.0);
    }
};

QTEST_GUILESS_MAIN(TstFeaturesCurtain)
#include "tst_features_curtain.moc"
