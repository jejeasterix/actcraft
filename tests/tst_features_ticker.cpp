#include "actcraft/features/ticker.h"

#include <QtTest>

using actcraft::features::tickerFromJson;
using actcraft::features::TickerRun;
using actcraft::features::TickerSettings;
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
        QCOMPARE(TickerSettings::positionFromText(QStringLiteral("bottom")),
                 TickerSettings::Position::Bottom);
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
        s.shadowColor = QColor(0x65, 0x43, 0x21);
        s.screenBackground = false;
        s.backgroundColor = QColor(0xab, 0xcd, 0xef);
        s.position = TickerSettings::Position::Bottom;
        s.loop = true;
        s.speed = 7;
        // Couleurs d'ombre et de fond distinctes des défauts : un champ perdu à l'aller-retour se verrait.
        QVERIFY(s.shadowColor != TickerSettings().shadowColor);
        QVERIFY(s.backgroundColor != TickerSettings().backgroundColor);
        const auto back = tickerFromJson(tickerToJson(s));
        QVERIFY(back.has_value());
        QVERIFY(*back == s);
        QCOMPARE(back->shadowColor, s.shadowColor);
        QCOMPARE(back->backgroundColor, s.backgroundColor);

        // La position centrale fait aussi l'aller-retour (les deux autres sont lues par positionsAreThree).
        s.position = TickerSettings::Position::Center;
        const auto centred = tickerFromJson(tickerToJson(s));
        QVERIFY(centred.has_value());
        QCOMPARE(centred->position, TickerSettings::Position::Center);

        TickerSettings big;
        big.text = QString(70 * 1024, QLatin1Char('a'));
        QVERIFY(!tickerFromJson(tickerToJson(big)).has_value()); // plus de 64 Kio : refusé

        QVERIFY(!tickerFromJson(R"({"format": "autre/1", "text": "x"})").has_value());
        QVERIFY(!tickerFromJson("pas du json").has_value());
    }
    // Revue qualité z1 : la taille et l'ombre lues d'un fichier ont la borne haute de la boîte de réglage
    // (200 points, 20 px) ; une valeur dans les bornes passe telle quelle.
    void fontSizeAndShadowAreBounded()
    {
        const auto huge = tickerFromJson(
            R"({"format": "actcraft-ticker/1", "text": "x", "fontPointSize": 5000, "shadowOffset": 999})");
        QVERIFY(huge.has_value());
        QCOMPARE(huge->fontPointSize, 200.0);
        QCOMPARE(huge->shadowOffset, 20);
        const auto inside = tickerFromJson(
            R"({"format": "actcraft-ticker/1", "text": "x", "fontPointSize": 64, "shadowOffset": 7})");
        QVERIFY(inside.has_value());
        QCOMPARE(inside->fontPointSize, 64.0);
        QCOMPARE(inside->shadowOffset, 7);
    }
    void unknownKeysAreIgnored()
    {
        const auto s = tickerFromJson(
            R"({"format": "actcraft-ticker/1", "text": "Bonjour", "inconnue": 3, "speed": 99})");
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
