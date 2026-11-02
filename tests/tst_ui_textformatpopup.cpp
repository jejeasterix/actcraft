#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

// L4-C20 : les fenêtres de choix de la barre de texte (couleur, fond, symboles) se referment par un second
// appui sur leur bouton. Avec CloseOnPressOutside, l'appui hors de la fenêtre la fermait puis le relâché sur
// le bouton la rouvrait.
class TstUiTextFormatPopup : public QObject
{
    Q_OBJECT

private slots:
    void secondPressOnTheButtonClosesTheWindow_data()
    {
        QTest::addColumn<QString>("button");
        QTest::newRow("couleur") << "textColorButton";
        QTest::newRow("fond") << "textBackgroundButton";
        QTest::newRow("symboles") << "textSymbolButton";
    }

    void secondPressOnTheButtonClosesTheWindow()
    {
        QFETCH(QString, button);
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(QByteArray("import QtQuick\nimport ActCraft.Ui\nItem {\n  width: 1600; height: 400\n"
                                     "  Commands { id: cmd }\n"
                                     "  TextFormatBar { objectName: \"bar\"; commands: cmd; width: 1600 }\n}\n"),
                          QUrl());
        std::unique_ptr<QObject> created(component.create());
        QVERIFY2(created, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(created.get());
        QVERIFY(root);
        QQuickWindow window;
        window.resize(1600, 400);
        root->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        QObject* bar = root->findChild<QObject*>(QStringLiteral("bar"));
        auto* choice = root->findChild<QQuickItem*>(button);
        QVERIFY(bar && choice);
        QVERIFY2(choice->isVisible() && choice->width() > 0, "bouton absent de la barre");
        const QPoint at = choice->mapToScene(QPointF(choice->width() / 2, choice->height() / 2)).toPoint();

        QCOMPARE(bar->property("openLists").toInt(), 0);
        QTest::mouseClick(&window, Qt::LeftButton, {}, at);
        QTRY_COMPARE(bar->property("openLists").toInt(), 1);
        QTest::mouseClick(&window, Qt::LeftButton, {}, at);
        QTRY_COMPARE(bar->property("openLists").toInt(), 0);
        QTest::qWait(100); // une réouverture tardive serait vue ici
        QCOMPARE(bar->property("openLists").toInt(), 0);
    }
};

QTEST_MAIN(TstUiTextFormatPopup)
#include "tst_ui_textformatpopup.moc"
