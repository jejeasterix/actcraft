#include "startup.h"

#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/pageview.h>

#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTabletEvent>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

struct Shell
{
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::PageView* view = nullptr;
    render::FixedPageItem* fixed = nullptr;
};

bool ready(Shell& s)
{
    return QTest::qWaitFor([&] { return s.fixed->isReady() && s.view->cacheReady(); }, 10000);
}

bool start(Shell& s)
{
    s.window = loadMainWindow(s.engine, StartupOptions{});
    if (!s.window || !QTest::qWaitForWindowExposed(s.window)) {
        return false;
    }
    s.view = s.window->findChild<ui::PageView*>(QStringLiteral("pageView"));
    s.fixed = s.window->findChild<render::FixedPageItem*>(QStringLiteral("fixedPage"));
    return s.view && s.fixed && ready(s);
}

// Les délégués d'un Repeater n'ont pas toujours la fenêtre pour parent QObject : on suit l'arbre des
// éléments.
QQuickItem* findItem(QQuickItem* root, const QString& name)
{
    if (!root) {
        return nullptr;
    }
    if (root->objectName() == name) {
        return root;
    }
    const QList<QQuickItem*> children = root->childItems();
    for (QQuickItem* child : children) {
        if (QQuickItem* found = findItem(child, name)) {
            return found;
        }
    }
    return nullptr;
}

QQuickItem* item(Shell& s, const char* name)
{
    return findItem(s.window->contentItem(), QString::fromLatin1(name));
}

bool run(Shell& s, const QString& id, const QVariant& argument = QVariant())
{
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(s.window, "runCommand", Q_RETURN_ARG(QVariant, result),
                                                   Q_ARG(QVariant, QVariant(id)), Q_ARG(QVariant, argument));
    return invoked && result.toBool();
}

QString indicator(Shell& s)
{
    QQuickItem* label = item(s, "pageIndicator");
    return label ? label->property("text").toString() : QString();
}

QColor pixelAtPage(Shell& s, QPointF page)
{
    const QImage image = s.window->grabWindow();
    const QPointF scene = s.view->pageToScene(page) * s.window->effectiveDevicePixelRatio();
    return image.pixelColor(scene.toPoint());
}

bool near(const QColor& a, const QColor& b, int tolerance)
{
    return qAbs(a.red() - b.red()) <= tolerance && qAbs(a.green() - b.green()) <= tolerance &&
           qAbs(a.blue() - b.blue()) <= tolerance;
}

} // namespace

// Coquille de l'interface (décision 4 du cadrage livraison-complete ; S2).
class TstUiShell : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QPointingDevice> m_stylus;

    void post(Shell& s, QEvent::Type type, QPointF scene, Qt::MouseButtons buttons)
    {
        QCoreApplication::postEvent(s.window,
                                    new QTabletEvent(type, m_stylus.get(), scene,
                                                     s.window->mapToGlobal(scene), buttons ? 0.5 : 0.0, 0, 0,
                                                     0, 0, 0, Qt::NoModifier, Qt::LeftButton, buttons));
    }

private slots:
    void initTestCase()
    {
        m_stylus = std::make_unique<QPointingDevice>(
            QStringLiteral("tst-ui-shell-stylus"), 5170, QInputDevice::DeviceType::Stylus,
            QPointingDevice::PointerType::Pen,
            QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3);
    }

    void shellShowsTopBarPaletteAndPage()
    {
        Shell s;
        QVERIFY(start(s));
        for (const char* name :
             {"topBar", "toolPalette", "pageArea", "pageIndicator", "zoomBox", "colorSwatches", "widthPicker",
              "menu-file", "menu-view", "menu-tools", "menu-help", "button-page.next", "button-page.previous",
              "button-tool.pen", "button-menu.main", "button-menu.tools", "button-view.fullscreen"}) {
            QQuickItem* found = item(s, name);
            QVERIFY2(found && found->isVisible(), name);
        }
        QVERIFY(!item(s, "menu-edit"));   // menu vide en lot 1 : pas affiché
        QVERIFY(!item(s, "menu-insert")); // idem
        QCOMPARE(indicator(s), QStringLiteral("1 / 1"));
        QCOMPARE(s.fixed->objectCount(), 0);
        // Palette à droite de la page, ligne supérieure au-dessus (F-001, F-002) ; pas de barre d'état
        // (F-003).
        QQuickItem* palette = item(s, "toolPalette");
        QQuickItem* bar = item(s, "topBar");
        QVERIFY(palette->mapToScene(QPointF(0, 0)).x() >=
                s.view->mapToScene(QPointF(s.view->width(), 0)).x() - 1);
        QVERIFY(bar->mapToScene(QPointF(0, bar->height())).y() <= s.view->mapToScene(QPointF(0, 0)).y() + 1);
        QVERIFY(s.view->mapToScene(QPointF(0, s.view->height())).y() >= s.window->height() - 1);
        QVERIFY(item(s, "button-tool.pen")->property("checked").toBool()); // F-200
    }

    void pageCommandsAddPageAndMoveTheIndicator()
    {
        Shell s;
        QVERIFY(start(s));
        QVERIFY(run(s, QStringLiteral("page.next")));
        QVERIFY(ready(s));
        QCOMPARE(indicator(s), QStringLiteral("2 / 2"));
        QVERIFY(run(s, QStringLiteral("page.previous")));
        QVERIFY(ready(s));
        QCOMPARE(indicator(s), QStringLiteral("1 / 2"));
    }

    void penCommandsSetColorAndWidth()
    {
        Shell s;
        QVERIFY(start(s));
        QVERIFY(run(s, QStringLiteral("pen.color"), QStringLiteral("#E53935")));
        QCOMPARE(s.view->penColor(), QColor(QStringLiteral("#E53935")));
        QVERIFY(run(s, QStringLiteral("pen.width"), 8));
        QCOMPARE(s.view->penWidth(), 8.0);
        QVERIFY(!run(s, QStringLiteral("pen.color"), QStringLiteral("rouge")));
        QCOMPARE(s.view->penColor(), QColor(QStringLiteral("#E53935")));
    }

    void redStrokeSurvivesPageRoundTrip()
    {
        Shell s;
        QVERIFY(start(s));
        QVERIFY(run(s, QStringLiteral("pen.color"), QStringLiteral("#E53935")));
        QVERIFY(run(s, QStringLiteral("pen.width"), 8));
        const QPoint a = s.view->pageToScene(QPointF(300, 400)).toPoint();
        const QPoint b = s.view->pageToScene(QPointF(700, 400)).toPoint();
        QTest::mousePress(s.window, Qt::LeftButton, Qt::NoModifier, a);
        for (int i = 1; i <= 20; ++i) {
            QTest::mouseMove(s.window, a + (b - a) * i / 20);
        }
        QTest::mouseRelease(s.window, Qt::LeftButton, Qt::NoModifier, b);
        QTRY_COMPARE_WITH_TIMEOUT(s.fixed->objectCount(), 1, 3000);
        QVERIFY(run(s, QStringLiteral("page.next")));
        QVERIFY(ready(s));
        QCOMPARE(s.fixed->objectCount(), 0);
        QVERIFY(run(s, QStringLiteral("page.previous")));
        QVERIFY(ready(s));
        QCOMPARE(s.fixed->objectCount(), 1);
        QVERIFY(near(pixelAtPage(s, QPointF(500, 400)), QColor(QStringLiteral("#E53935")), 60));
    }

    void f5TogglesFullScreen()
    {
        Shell s;
        QVERIFY(start(s));
        QTest::keyClick(s.window, Qt::Key_F5);
        QTRY_VERIFY(s.window->property("fullScreen").toBool());
        QTest::keyClick(s.window, Qt::Key_F5);
        QTRY_VERIFY(!s.window->property("fullScreen").toBool());
    }

    void swatchTapChoosesThePenColor()
    {
        Shell s;
        QVERIFY(start(s));
        QQuickItem* swatch = item(s, "swatch-5");
        QVERIFY(swatch);
        const QPointF center = swatch->mapToScene(QPointF(swatch->width() / 2, swatch->height() / 2));
        QTest::mouseClick(s.window, Qt::LeftButton, Qt::NoModifier, center.toPoint());
        QTRY_COMPARE(s.view->penColor(), QColor(QStringLiteral("#1E88E5")));
    }

    void stylusOnThePaletteDoesNotDraw()
    {
        Shell s;
        QVERIFY(start(s));
        QQuickItem* palette = item(s, "toolPalette");
        const QPointF a = palette->mapToScene(QPointF(palette->width() / 2, palette->height() - 12));
        post(s, QEvent::TabletPress, a, Qt::LeftButton);
        post(s, QEvent::TabletMove, a - QPointF(300, 0), Qt::LeftButton);
        post(s, QEvent::TabletRelease, a - QPointF(300, 0), Qt::NoButton);
        QTest::qWait(150);
        QCOMPARE(s.fixed->objectCount(), 0);
    }

    void aboutCommandOpensTheDialogAndBlocksThePage()
    {
        Shell s;
        QVERIFY(start(s));
        QVERIFY(run(s, QStringLiteral("help.about")));
        QObject* dialog = s.window->findChild<QObject*>(QStringLiteral("aboutDialog"));
        QVERIFY(dialog);
        QTRY_VERIFY(dialog->property("opened").toBool());
        QVERIFY(s.view->inputBlocked());
        QVERIFY(QMetaObject::invokeMethod(dialog, "close"));
        QTRY_VERIFY(!s.view->inputBlocked());
    }
};

QTEST_MAIN(TstUiShell)
#include "tst_ui_shell.moc"
