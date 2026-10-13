#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/pageview.h>

#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

// La fenêtre principale se charge et s'affiche avec le rendu logiciel de secours.
class TstUi : public QObject
{
    Q_OBJECT

private slots:
    void mainWindowLoadsWithSoftwareRenderer()
    {
        QQmlApplicationEngine engine;
        engine.loadFromModule("ActCraft.Ui", "Main");
        QCOMPARE(engine.rootObjects().size(), 1);

        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
        QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowExposed(window));
        QCOMPARE(window->rendererInterface()->graphicsApi(), QSGRendererInterface::Software);

        auto* page = window->findChild<QQuickItem*>(QStringLiteral("page"));
        QVERIFY(page);
        QCOMPARE(page->width(), 1024.0);
        QCOMPARE(page->height(), 768.0);

        auto* view = window->findChild<actcraft::ui::PageView*>(QStringLiteral("pageView"));
        auto* fixed = window->findChild<actcraft::render::FixedPageItem*>(QStringLiteral("fixedPage"));
        QVERIFY(view && fixed);
        QTRY_VERIFY_WITH_TIMEOUT(fixed->isReady() && view->cacheReady(), 20000);
        QCOMPARE(fixed->objectCount(), 300);
        QVERIFY(window->title().contains(QStringLiteral("logiciel")));
    }
};

QTEST_MAIN(TstUi)
#include "tst_ui.moc"
