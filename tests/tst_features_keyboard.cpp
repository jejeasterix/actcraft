#include <actcraft/features/screenkeyboard.h>

#include <QtTest>

using namespace actcraft::features;

class TstFeaturesKeyboard : public QObject
{
    Q_OBJECT

private slots:
    void shiftTypesCapitals()
    {
        ScreenKeyboard k(ScreenKeyboard::Layout::Standard);
        QCOMPARE(k.press(QStringLiteral("a")).text, QStringLiteral("a"));
        k.press(QStringLiteral("shift"));
        QCOMPARE(k.press(QStringLiteral("a")).text, QStringLiteral("A"));
        QCOMPARE(k.press(QStringLiteral("a")).text, QStringLiteral("a")); // majuscule pour une touche
    }

    void capsLockStays()
    {
        ScreenKeyboard k(ScreenKeyboard::Layout::Standard);
        k.press(QStringLiteral("caps"));
        QCOMPARE(k.press(QStringLiteral("z")).text, QStringLiteral("Z"));
        QCOMPARE(k.press(QStringLiteral("z")).text, QStringLiteral("Z"));
    }

    void compactLayoutHasFewerKeys()
    {
        QVERIFY(ScreenKeyboard(ScreenKeyboard::Layout::Compact).keys().size()
                < ScreenKeyboard(ScreenKeyboard::Layout::Extended).keys().size());
    }

    void accentKeysTypeFrench()
    {
        ScreenKeyboard k(ScreenKeyboard::Layout::Extended);
        QCOMPARE(k.press(QStringLiteral("é")).text, QStringLiteral("é"));
        QCOMPARE(k.press(QStringLiteral("backspace")).key, Qt::Key_Backspace);
    }

    void everyKeyHasALabel()
    {
        for (auto layout : {ScreenKeyboard::Layout::Compact, ScreenKeyboard::Layout::Standard, ScreenKeyboard::Layout::Extended}) {
            const ScreenKeyboard k(layout);
            for (const auto& cap : k.keys()) {
                QVERIFY(!cap.id.isEmpty());
                QVERIFY(!cap.label.isEmpty());
                QVERIFY(cap.width > 0);
            }
        }
    }
};

QTEST_GUILESS_MAIN(TstFeaturesKeyboard)
#include "tst_features_keyboard.moc"
