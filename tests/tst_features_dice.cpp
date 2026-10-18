#include <actcraft/features/dice.h>

#include <QtTest>

using actcraft::features::Dice;

class TstFeaturesDice : public QObject
{
    Q_OBJECT

private slots:
    void seededRollIsReproducible()
    {
        Dice a(42), b(42);
        a.setCount(5);
        b.setCount(5);
        QCOMPARE(a.roll(), b.roll());
    }
    void countClampedTo1To5()
    {
        Dice d(1);
        d.setCount(0);
        QCOMPARE(d.count(), 1);
        d.setCount(9);
        QCOMPARE(d.count(), 5);
    }
    void totalIsTheSum()
    {
        Dice d(7);
        d.setCount(3);
        const auto faces = d.roll();
        QCOMPARE(faces.size(), std::size_t{3});
        int sum = 0;
        for (int f : faces) {
            QVERIFY(f >= 1 && f <= 6);
            sum += f;
        }
        QCOMPARE(d.total(), sum);
    }
    void speedClampedTo3To11()
    {
        QCOMPARE(Dice::animationMs(1), 3000);
        QCOMPARE(Dice::animationMs(7), 7000);
        QCOMPARE(Dice::animationMs(20), 11000);
    }
    void defaultIsOneDie()
    {
        // E5, L7-T0 (docs/verifications/2026-10-18-lot7-principes.md) : 1 dé au premier lancement ; vitesse 7
        // s.
        const Dice d(3);
        QCOMPARE(d.count(), 1);
        QCOMPARE(Dice::kDefaultSeconds, 7);
    }
};

QTEST_GUILESS_MAIN(TstFeaturesDice)
#include "tst_features_dice.moc"
