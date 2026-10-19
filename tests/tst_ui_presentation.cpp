#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <QtTest>

#include <utility>

namespace {

class TstUiPresentation : public QObject
{
    Q_OBJECT

private slots:
    void lot7RootsExist()
    {
        presentationharness::MainWindow main;
        QVERIFY(main.window() != nullptr);
        // QtObject, pas un Item (socle L0-S9).
        QObject* presentation = main.window()->findChild<QObject*>(QStringLiteral("presentation"));
        QVERIFY(presentation != nullptr);
        const char* roots[] = {"curtainRoot",    "spotlightRoot", "pageZoomRoot",  "pageToolsRoot",
                               "transitionRoot", "effectsRoot",   "mathToolsRoot", "diceRoot",
                               "calculatorRoot", "clockRoot",     "tickerRoot",    "recognitionRoot",
                               "equationRoot",   "cameraRoot",    "keyboardRoot",  "desktopRoot",
                               "otherToolsRoot"};
        for (const char* name : roots) {
            QObject* root = main.window()->findChild<QObject*>(QString::fromLatin1(name));
            QVERIFY2(root != nullptr, name);
            // Les quatre liaisons posées par Presentation.qml (décision 4).
            for (const char* bound : {"commands", "documentController", "tools", "pageArea"}) {
                QVERIFY2(root->property(bound).value<QObject*>() != nullptr,
                         qPrintable(QStringLiteral("%1.%2").arg(QLatin1String(name), QLatin1String(bound))));
            }
        }
        auto* overlays = main.window()->findChild<QQuickItem*>(QStringLiteral("pageOverlays"));
        QVERIFY(overlays != nullptr);
        for (const char* name : {"curtainRoot", "spotlightRoot", "mathToolsRoot"}) {
            auto* root = main.window()->findChild<QQuickItem*>(QString::fromLatin1(name));
            QVERIFY2(root != nullptr && root->parentItem() == overlays, name);
        }
        auto* layer = main.window()->findChild<QQuickItem*>(QStringLiteral("presentationLayer"));
        auto* ticker = main.window()->findChild<QQuickItem*>(QStringLiteral("tickerRoot"));
        QVERIFY(layer != nullptr && ticker != nullptr);
        QCOMPARE(ticker->parentItem(), layer);
        // Empilement fixé par z (décision 3) : chaque valeur, pas seulement un ordre.
        const std::pair<const char*, qreal> stacking[] = {
            {"curtainRoot", 10},     {"spotlightRoot", 20}, {"mathToolsRoot", 30},  {"transitionRoot", 10},
            {"tickerRoot", 20},      {"diceRoot", 30},      {"calculatorRoot", 31}, {"clockRoot", 32},
            {"recognitionRoot", 40}, {"equationRoot", 41},  {"effectsRoot", 50}};
        for (const auto& [name, z] : stacking) {
            auto* root = main.window()->findChild<QQuickItem*>(QString::fromLatin1(name));
            QVERIFY2(root != nullptr, name);
            QVERIFY2(qFuzzyCompare(root->z(), z), name);
        }
        for (const char* name : {"cameraRoot", "keyboardRoot", "desktopRoot"}) {
            auto* root = main.window()->findChild<QQuickItem*>(QString::fromLatin1(name));
            QVERIFY2(root != nullptr && !root->isVisible(), name); // racine non visuelle (décision 4 bis)
        }
        // Parent visuel de chacune des 17 racines (décision 3) : 12 placées, 5 sans parent.
        auto* slot = main.window()->findChild<QQuickItem*>(QStringLiteral("presentationSlot"));
        QVERIFY(slot != nullptr);
        const std::pair<const char*, QQuickItem*> parents[] = {
            {"curtainRoot", overlays},  {"spotlightRoot", overlays}, {"mathToolsRoot", overlays},
            {"pageZoomRoot", slot},     {"transitionRoot", layer},   {"effectsRoot", layer},
            {"diceRoot", layer},        {"calculatorRoot", layer},   {"clockRoot", layer},
            {"tickerRoot", layer},      {"recognitionRoot", layer},  {"equationRoot", layer},
            {"pageToolsRoot", nullptr}, {"otherToolsRoot", nullptr}, {"cameraRoot", nullptr},
            {"keyboardRoot", nullptr},  {"desktopRoot", nullptr}};
        for (const auto& [name, expected] : parents) {
            auto* root = main.window()->findChild<QQuickItem*>(QString::fromLatin1(name));
            QVERIFY2(root != nullptr && root->parentItem() == expected, name);
        }
    }

    void reserved() { QSKIP("Talon L7-S8 : rempli par L7-T2."); }
};

} // namespace

ACTCRAFT_LOT7_TEST("presentation", TstUiPresentation)
#include "tst_ui_presentation.moc"
