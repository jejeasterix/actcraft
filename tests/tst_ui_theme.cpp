#include <QColor>
#include <QFile>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSvgRenderer>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

namespace {

const char* const kIcons[] = {"menu",        "outils",  "page-precedente", "page-suivante", "stylo",
                              "plein-ecran", "ajuster", "quitter",         "aide",          "a-propos"};

} // namespace

// Thème et icônes à nous (décision 23).
class TstUiTheme : public QObject
{
    Q_OBJECT

private slots:
    void themeSingletonExposesTokens()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport ActCraft.Ui\nQtObject {\n"
                          "  property color fond: Theme.fond\n"
                          "  property color palette: Theme.palette\n"
                          "  property color texte: Theme.texte\n"
                          "  property int bouton: Theme.tailleBouton\n"
                          "  property int largeurPalette: Theme.largeurPalette\n"
                          "  property int hauteurBarre: Theme.hauteurBarre\n"
                          "}\n",
                          QUrl());
        std::unique_ptr<QObject> tokens(component.create());
        QVERIFY2(tokens, qPrintable(component.errorString()));
        QVERIFY(tokens->property("fond").value<QColor>().isValid());
        QVERIFY(tokens->property("palette").value<QColor>().isValid());
        QVERIFY(tokens->property("texte").value<QColor>().isValid());
        QVERIFY(tokens->property("bouton").toInt() >= 44); // cible tactile
        QVERIFY(tokens->property("largeurPalette").toInt() >= 96);
        QVERIFY(tokens->property("hauteurBarre").toInt() >= 32);
    }

    void iconsAreOurSvgOnA24Grid_data()
    {
        QTest::addColumn<QString>("name");
        for (const char* name : kIcons) {
            QTest::newRow(name) << QString::fromLatin1(name);
        }
    }

    void iconsAreOurSvgOnA24Grid()
    {
        QFETCH(QString, name);
        const QString path = QStringLiteral(":/qt/qml/ActCraft/Ui/icons/%1.svg").arg(name);
        QVERIFY2(QFile::exists(path), qPrintable(path));
        QSvgRenderer renderer(path);
        QVERIFY(renderer.isValid());
        QCOMPARE(renderer.viewBoxF(), QRectF(0, 0, 24, 24));
    }
};

QTEST_MAIN(TstUiTheme)
#include "tst_ui_theme.moc"
