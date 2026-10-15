#include <QQmlComponent>
#include <QQmlEngine>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

// « À propos » et identité de l'application (F-760, décision 34).
class TstUiAbout : public QObject
{
    Q_OBJECT

private slots:
    void appInfoGivesVersionLicenseAndNotices()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport ActCraft.Ui\nQtObject {\n"
                          "  property string version: AppInfo.version\n"
                          "  property string licence: AppInfo.licenseName\n"
                          "  property string url: AppInfo.projectUrl\n"
                          "  property string notices: AppInfo.notices\n"
                          "}\n",
                          QUrl());
        std::unique_ptr<QObject> info(component.create());
        QVERIFY2(info, qPrintable(component.errorString()));
        QCOMPARE(info->property("version").toString(), QStringLiteral(ACTCRAFT_TEST_VERSION));
        QCOMPARE(info->property("licence").toString(), QStringLiteral("MIT"));
        QCOMPARE(info->property("url").toString(), QStringLiteral("https://github.com/jejeasterix/actcraft"));
        const QString notices = info->property("notices").toString();
        QVERIFY2(notices.contains(QLatin1String("Qt")) && notices.contains(QLatin1String("LGPL")),
                 "THIRD_PARTY_NOTICES.md absent des ressources");
    }

    void aboutDialogLoadsWithOurTitle()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport ActCraft.Ui\nWindow {\n"
                          "  width: 800; height: 600\n"
                          "  property QtObject dialogue: about\n"
                          "  AboutDialog { id: about }\n"
                          "}\n",
                          QUrl());
        std::unique_ptr<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        QObject* dialog = window->property("dialogue").value<QObject*>();
        QVERIFY(dialog);
        QCOMPARE(dialog->objectName(), QStringLiteral("aboutDialog"));
        QCOMPARE(dialog->property("title").toString(), QStringLiteral("À propos d'ActCraft"));
    }
};

QTEST_MAIN(TstUiAbout)
#include "tst_ui_about.moc"
