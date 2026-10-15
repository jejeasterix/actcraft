#include <QFont>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

// Les textes de l'interface rendent de vrais glyphes, y compris hors écran (L1-C2). Sans base de polices, Qt
// dessine un carré vide identique pour chaque caractère : toutes les largeurs de texte deviennent égales.
class TstUiPolice : public QObject
{
    Q_OBJECT

private slots:
    void fontDatabaseIsNotEmpty()
    {
        QVERIFY2(
            !QFontDatabase::families().isEmpty(),
            "aucune famille de polices : textes rendus en carrés vides (plateforme hors écran sans polices)");
    }

    void defaultFontHasRealGlyphs()
    {
        const QFontMetricsF metrics(QGuiApplication::font());
        QVERIFY2(metrics.inFontUcs4(U'F'), "la police par défaut ne contient pas la lettre F");
        QVERIFY2(metrics.horizontalAdvance(QStringLiteral("iiiiiii")) <
                     metrics.horizontalAdvance(QStringLiteral("WWWWWWW")),
                 "tous les caractères ont la même largeur : glyphe de remplacement");
    }

    void shellTextIsNotTofu()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(
            "import QtQuick\nimport ActCraft.Ui\nQtObject {\n"
            "  property Text fin: Text { text: \"iiiiiii\"; font.pixelSize: Theme.taillePolice }\n"
            "  property Text large: Text { text: \"WWWWWWW\"; font.pixelSize: Theme.taillePolice }\n"
            "}\n",
            QUrl());
        std::unique_ptr<QObject> objects(component.create());
        QVERIFY2(objects, qPrintable(component.errorString()));
        const double thin = objects->property("fin").value<QObject*>()->property("contentWidth").toDouble();
        const double wide = objects->property("large").value<QObject*>()->property("contentWidth").toDouble();
        QVERIFY2(thin > 0 && thin < wide, "le texte de l'interface est rendu en carrés vides");
    }
};

QTEST_MAIN(TstUiPolice)
#include "tst_ui_police.moc"
