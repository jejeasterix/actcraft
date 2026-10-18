#include <actcraft/ui/toolhost.h>

#include <QQmlComponent>
#include <QQmlEngine>
#include <QScopedPointer>
#include <QTest>
#include <QVariant>

class TstUiTools : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L0-S : rempli par L3-T14."); }

    void oneUserForNow()
    {
        actcraft::ui::ToolHost host;
        actcraft::tools::UserState& a = host.user();
        actcraft::tools::UserState& b = host.user(actcraft::input::kFirstUser);
        QCOMPARE(&a, &b);
        QVERIFY(host.activeTool() == nullptr); // talon jusqu'à L3-T14
    }

    void liftLayerTypeIsRegistered()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport ActCraft.Ui\nLiftLayer {}\n", QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
    }

    // H-S2 : une propriété requise accepte null comme valeur initiale (talons QML à required property).
    void requiredPropertyAcceptsNull()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nItem { required property Item t }\n", QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.createWithInitialProperties(
            {{QStringLiteral("t"), QVariant::fromValue<QObject*>(nullptr)}}));
        QVERIFY2(object, qPrintable(component.errorString()));
        QVERIFY(component.errors().isEmpty());
    }
};

QTEST_MAIN(TstUiTools)
#include "tst_ui_tools.moc"
