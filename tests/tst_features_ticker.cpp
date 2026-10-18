#include <QtTest>

#include "actcraft/features/ticker.h"

using actcraft::features::TickerRun;
using actcraft::features::TickerSettings;
using actcraft::features::tickerFromJson;
using actcraft::features::tickerToJson;

class TstFeaturesTicker : public QObject
{
    Q_OBJECT

private slots:
    void speedMapsToPixelsPerSecond()
    {
        QCOMPARE(TickerSettings::pixelsPerSecond(1), 40.0);
        QCOMPARE(TickerSettings::pixelsPerSecond(10), 400.0);
        QCOMPARE(TickerSettings::pixelsPerSecond(99), 400.0);
    }
    void singlePassEndsHidden()
    {
        TickerRun run(1000, 200, 100.0, false); // largeur d'écran, largeur du texte, px/s, boucle
        QCOMPARE(run.durationMs(), 12000);      // (1000 + 200) / 100 s
        QCOMPARE(run.xAt(0), 1000.0);
        QVERIFY(run.finishedAt(12000));
    }
    void loopRestarts()
    {
        TickerRun run(1000, 200, 100.0, true);
        QVERIFY(!run.finishedAt(50000));
        QCOMPARE(run.xAt(12000), 1000.0);
    }
    void positionsAreThree()
    {
        QCOMPARE(TickerSettings::positionFromText(QStringLiteral("bottom")), TickerSettings::Position::Bottom);
        QCOMPARE(TickerSettings::positionFromText(QStringLiteral("?")), TickerSettings::Position::Top);
    }
    void messageFileRoundTrip()
    {
        TickerSettings s;
        s.text = QStringLiteral("Récréation à 10 h <b>pas de gras</b>");
        s.fontFamily = QStringLiteral("Arial");
        s.fontPointSize = 40;
        s.textColor = QColor(0x12, 0x34, 0x56);
        s.shadowOffset = 3;
        s.screenBackground = false;
        s.position = TickerSettings::Position::Bottom;
        s.loop = true;
        s.speed = 7;
        const auto back = tickerFromJson(tickerToJson(s));
        QVERIFY(back.has_value());
        QVERIFY(*back == s);

        TickerSettings big;
        big.text = QString(70 * 1024, QLatin1Char('a'));
        QVERIFY(!tickerFromJson(tickerToJson(big)).has_value()); // plus de 64 Kio : refusé

        QVERIFY(!tickerFromJson(R"({"format": "autre/1", "text": "x"})").has_value());
        QVERIFY(!tickerFromJson("pas du json").has_value());
    }
    void unknownKeysAreIgnored()
    {
        const auto s =
            tickerFromJson(R"({"format": "actcraft-ticker/1", "text": "Bonjour", "inconnue": 3, "speed": 99})");
        QVERIFY(s.has_value());
        QCOMPARE(s->text, QStringLiteral("Bonjour"));
        QCOMPARE(s->speed, 10);                               // borné
        QCOMPARE(s->position, TickerSettings::Position::Top); // absente : défaut
    }
    void loopNeverFinishes()
    {
        TickerRun run(1000, 200, 100.0, true);
        QCOMPARE(run.xAt(6000), 400.0);
        QVERIFY(!run.finishedAt(1'000'000));
    }
};

QTEST_GUILESS_MAIN(TstFeaturesTicker)
#include "tst_features_ticker.moc"
