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
        QCOMPARE(info->property("licence").toString(), QStringLiteral("GNU GPL v3"));
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

    // GPLv3 § 0 et § 5 d : l'interface montre les « mentions légales appropriées » (copyright, absence de
    // garantie, droit de redistribuer sous cette licence, moyen de lire la licence).
    void appInfoGivesCopyrightAndLicenseText()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport ActCraft.Ui\nQtObject {\n"
                          "  property string copyright: AppInfo.copyright\n"
                          "  property string texte: AppInfo.licenseText\n"
                          "}\n",
                          QUrl());
        std::unique_ptr<QObject> info(component.create());
        QVERIFY2(info, qPrintable(component.errorString()));
        QCOMPARE(info->property("copyright").toString(), QStringLiteral("Copyright (C) 2026 Jérôme Denis"));
        const QString text = info->property("texte").toString();
        QVERIFY2(text.contains(QLatin1String("GNU GENERAL PUBLIC LICENSE")) &&
                     text.contains(QLatin1String("Version 3, 29 June 2007")) &&
                     text.contains(QLatin1String("Disclaimer of Warranty")),
                 "texte de la GPLv3 absent des ressources");
    }

    void aboutDialogShowsLegalNoticesAndTheLicense()
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
        QObject* legal = dialog->findChild<QObject*>(QStringLiteral("aboutLegal"));
        QVERIFY(legal);
        const QString legalText = legal->property("text").toString();
        QVERIFY2(legalText.contains(QStringLiteral("Copyright (C) 2026 Jérôme Denis")),
                 qPrintable(legalText));
        QVERIFY2(legalText.contains(QStringLiteral("sans aucune garantie")), qPrintable(legalText));
        QVERIFY2(legalText.contains(QStringLiteral("GNU General Public License")), qPrintable(legalText));

        QObject* notices = dialog->findChild<QObject*>(QStringLiteral("aboutNotices"));
        QObject* button = dialog->findChild<QObject*>(QStringLiteral("aboutLicenseButton"));
        QVERIFY(notices && button);
        QVERIFY(notices->property("text").toString().contains(QLatin1String("LGPL")));
        QVERIFY(QMetaObject::invokeMethod(button, "click"));
        QVERIFY(notices->property("text").toString().contains(QLatin1String("GNU GENERAL PUBLIC LICENSE")));
        QVERIFY(QMetaObject::invokeMethod(button, "click")); // retour aux notices des composants tiers
        QVERIFY(!notices->property("text").toString().contains(QLatin1String("Version 3, 29 June 2007")));
    }
};

QTEST_MAIN(TstUiAbout)
#include "tst_ui_about.moc"
