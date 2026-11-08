#include <actcraft/features/countdown.h>

#include <QtTest>

using namespace actcraft::features;

class TstFeaturesCountdown : public QObject
{
    Q_OBJECT

private slots:
    void endTriggersActionOnce()
    {
        Countdown c;
        c.start(3, 0); // 3 s, à t = 0 ns
        QCOMPARE(c.remainingSeconds(2'000'000'000), 1);
        QVERIFY(!c.poll(2'900'000'000)); // pas encore
        QVERIFY(c.poll(3'000'000'000));  // une fin
        QVERIFY(!c.poll(3'500'000'000)); // pas deux
    }
    void repeatRestarts()
    {
        Countdown c;
        c.setRepeat(true);
        c.start(2, 0);
        QVERIFY(c.poll(2'000'000'000));
        QCOMPARE(c.remainingSeconds(2'500'000'000), 2); // reparti à 2 s (arrondi au supérieur)
        QVERIFY(c.poll(4'000'000'000));
    }
    void pauseFreezes()
    {
        Countdown c;
        c.start(10, 0);
        c.pause(2'000'000'000);
        QCOMPARE(c.remainingSeconds(9'000'000'000), 8);
        c.resume(9'000'000'000);
        QCOMPARE(c.remainingSeconds(10'000'000'000), 7);
    }
    void formatHmsRoundTrip()
    {
        QCOMPARE(formatHms(3725), QStringLiteral("01:02:05"));
        QCOMPARE(parseHms(QStringLiteral("00:00:03")), 3);
        QCOMPARE(parseHms(QStringLiteral("xx")), -1);
    }
    void stopwatchCountsUp()
    {
        Stopwatch w;
        w.start(0);
        QCOMPARE(w.elapsedSeconds(65'000'000'000), 65);
    }
    void latePollFiresOnce()
    {
        Countdown c;
        c.setRepeat(true);
        c.start(2, 0);
        QVERIFY(c.poll(7'000'000'000)); // trois fins manquées : une seule annoncée
        QVERIFY(!c.poll(7'100'000'000));
        QCOMPARE(c.remainingSeconds(7'000'000'000), 1); // prochaine fin à 8 s
    }
};

QTEST_GUILESS_MAIN(TstFeaturesCountdown)
#include "tst_features_countdown.moc"
