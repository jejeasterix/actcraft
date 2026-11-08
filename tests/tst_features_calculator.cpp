#include <actcraft/features/calculator.h>

#include <QtTest>

using actcraft::features::Calculator;

class TstFeaturesCalculator : public QObject
{
    Q_OBJECT

private slots:
    void operatorPrecedenceLeftToRight() // calculatrice de poche : immédiat, de gauche à droite
    {
        Calculator c;
        for (const char* k : {"2", "+", "3", "*", "4", "="})
            c.press(QString::fromLatin1(k));
        QCOMPARE(c.display(), QStringLiteral("20"));
    }
    void decimalComma()
    {
        Calculator c;
        for (const char* k : {"1", ",", "5", "+", "1", "="})
            c.press(QString::fromLatin1(k));
        QCOMPARE(c.display(), QStringLiteral("2,5"));
    }
    void divisionByZeroIsAnError()
    {
        Calculator c;
        for (const char* k : {"5", "/", "0", "="})
            c.press(QString::fromLatin1(k));
        QCOMPARE(c.display(), QStringLiteral("Erreur"));
        c.press(QStringLiteral("C"));
        QCOMPARE(c.display(), QStringLiteral("0"));
    }
    void percentAndSign()
    {
        Calculator c;
        for (const char* k : {"2", "0", "0", "*", "5", "%"})
            c.press(QString::fromLatin1(k));
        QCOMPARE(c.display(), QStringLiteral("10"));
        c.press(QStringLiteral("±"));
        QCOMPARE(c.display(), QStringLiteral("-10"));
    }
    void memoryKeys()
    {
        Calculator c;
        for (const char* k : {"7", "M+", "C", "MR"})
            c.press(QString::fromUtf8(k));
        QCOMPARE(c.display(), QStringLiteral("7"));
    }
    void percentOfASum()
    {
        Calculator c;
        for (const char* k : {"2", "0", "0", "+", "5", "%"})
            c.press(QString::fromLatin1(k));
        QCOMPARE(c.display(), QStringLiteral("210")); // 200 + 5 % de 200
    }
    // P1, P2, P3 de L7-T0 (docs/verifications/2026-10-18-lot7-principes.md) : calcul affiché au-dessus du
    // résultat, deux effacements, calcul et résultat posés sur la page.
    void expressionShownAboveTheResult()
    {
        Calculator c;
        for (const char* k : {"1", "2", "+", "3"})
            c.press(QString::fromLatin1(k));
        QCOMPARE(c.expression(), QStringLiteral("12 +"));
        QCOMPARE(c.display(), QStringLiteral("3"));
        c.press(QStringLiteral("*"));
        QCOMPARE(c.expression(), QStringLiteral("12 + 3 ×"));
        c.press(QStringLiteral("/")); // deux opérations de suite : la seconde remplace la première
        QCOMPARE(c.expression(), QStringLiteral("12 + 3 ÷"));
    }
    void clearEntryKeepsTheCalculation()
    {
        Calculator c;
        for (const char* k : {"2", "+", "5", "CE", "3", "="})
            c.press(QString::fromLatin1(k));
        QCOMPARE(c.display(), QStringLiteral("5"));
        QCOMPARE(c.expression(), QStringLiteral("2 + 3 ="));
        c.press(QStringLiteral("C"));
        QCOMPARE(c.expression(), QString());
        QCOMPARE(c.display(), QStringLiteral("0"));
    }
    void pageTextHasTheCalculationAndItsResult()
    {
        Calculator c;
        QCOMPARE(c.pageText(), QStringLiteral("0"));
        for (const char* k : {"1", ",", "5", "+", "1", "="})
            c.press(QString::fromLatin1(k));
        QCOMPARE(c.pageText(), QStringLiteral("1,5 + 1 = 2,5"));
        c.press(QStringLiteral("4")); // nouveau calcul
        QCOMPARE(c.pageText(), QStringLiteral("4"));
    }
};

QTEST_GUILESS_MAIN(TstFeaturesCalculator)
#include "tst_features_calculator.moc"
