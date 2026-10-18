#include <actcraft/features/spotlight.h>

#include <QtTest>

using namespace actcraft::features;

class TstFeaturesSpotlight : public QObject
{
    Q_OBJECT
private slots:
    void defaultFrameIsCentered()
    {
        Spotlight s(QSizeF(1000, 800));
        s.setShape(Spotlight::Shape::Circle);
        QCOMPARE(s.frame().center(), QPointF(500, 400));
        QCOMPARE(s.frame().width(), 300.0); // kDefaultSize
    }
    void dragInsideMaskMoves()
    {
        Spotlight s(QSizeF(1000, 800));
        s.setShape(Spotlight::Shape::Circle);
        QVERIFY(s.press(QPointF(100, 100))); // dans le masque
        s.move(QPointF(150, 120));
        s.release();
        QCOMPARE(s.frame().center(), QPointF(550, 420));
    }
    void dragOnRimResizesToOval()
    {
        Spotlight s(QSizeF(1000, 800));
        s.setShape(Spotlight::Shape::Circle);
        QVERIFY(s.press(QPointF(650, 400))); // bord droit du cadre (500 + 150)
        s.move(QPointF(750, 400));
        s.release();
        QCOMPARE(s.frame().width(), 400.0);
        QCOMPARE(s.frame().height(), 300.0);
    }
    void minimumSize()
    {
        Spotlight s(QSizeF(1000, 800));
        s.setShape(Spotlight::Shape::Square);
        s.press(QPointF(650, 400));
        s.move(QPointF(0, 400));
        QVERIFY(s.frame().width() >= Spotlight::kMinimumSize);
    }
    void solidSpotHidesInside()
    {
        Spotlight s(QSizeF(1000, 800));
        s.setShape(Spotlight::Shape::SolidCircle);
        QVERIFY(s.hidesInside());
        QVERIFY(s.press(QPointF(500, 400))); // cache plein : on le saisit par l'intérieur
    }
    void frameKeptInsideTheArea()
    {
        Spotlight s(QSizeF(1000, 800));
        s.setShape(Spotlight::Shape::Circle);
        s.press(QPointF(100, 100));
        s.move(QPointF(-5000, -5000));
        QVERIFY(QRectF(0, 0, 1000, 800).intersects(s.frame()));
    }
};

QTEST_GUILESS_MAIN(TstFeaturesSpotlight)
#include "tst_features_spotlight.moc"
