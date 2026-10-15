#include "startup.h"

#include <actcraft/core/document.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>

#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTabletEvent>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>
#include <variant>

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

    void widthSliderFollowsThePenWidth()
    {
        Shell s;
        QVERIFY(start(s));
        QQuickItem* slider = item(s, "widthSlider");
        QVERIFY(slider);
        // Quelle que soit l'origine du changement, la réglette reflète l'épaisseur : valeur et position.
        QVERIFY(run(s, QStringLiteral("pen.width"), 8));
        QTRY_VERIFY(slider->property("visualPosition").toDouble() > 0.3);
        const double positionAt8 = slider->property("visualPosition").toDouble();
        QVERIFY2(positionAt8 > 0.3, qPrintable(QString::number(positionAt8)));
        QVERIFY(run(s, QStringLiteral("pen.width"), 100));
        QTRY_COMPARE(slider->property("value").toDouble(), 1.0);
        QCOMPARE(slider->property("visualPosition").toDouble(), 1.0);
        // Un préréglage cliqué (la plus forte) ramène la réglette à droite des plus faibles.
        QQuickItem* strong = item(s, "widthPreset-3");
        QVERIFY(strong);
        const QPoint c = strong->mapToScene(QPointF(strong->width() / 2, strong->height() / 2)).toPoint();
        QTest::mouseClick(s.window, Qt::LeftButton, Qt::NoModifier, c);
        QTRY_COMPARE(s.view->penWidth(), 8.0);
        QTRY_VERIFY(slider->property("visualPosition").toDouble() > 0.3);
        // Sens inverse : déplacer la réglette change l'épaisseur du stylo.
        const QPointF origin = slider->mapToScene(QPointF(0, 0));
        const QPoint mid = (origin + QPointF(slider->width() / 2, slider->height() / 2)).toPoint();
        QTest::mouseClick(s.window, Qt::LeftButton, Qt::NoModifier, mid);
        QTRY_VERIFY2(s.view->penWidth() > 8.0 && s.view->penWidth() < 100.0,
                     qPrintable(QString::number(s.view->penWidth())));
        // L'échantillon montre l'épaisseur du stylo, lisible (au moins 8 px pour 8).
        QQuickItem* sample = item(s, "widthIndicatorSample");
        QVERIFY(sample);
        QVERIFY2(sample->width() >= 12, qPrintable(QString::number(sample->width())));
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

    // Zoom 200 % : les barres de défilement sont posées sur la page. Le stylet appuyé sur la barre ne trace
    // pas : il est laissé à la barre, comme la souris et le doigt.
    void stylusOnAScrollBarDoesNotDraw()
    {
        Shell s;
        QVERIFY(start(s));
        QVERIFY(run(s, QStringLiteral("view.zoom"), 200));
        QVERIFY(ready(s));
        QQuickItem* bar = item(s, "horizontalScrollBar");
        QVERIFY(bar);
        QTRY_VERIFY(bar->isVisible());
        const QPointF a = bar->mapToScene(QPointF(bar->width() / 2, bar->height() / 2));
        QVERIFY(s.view->contains(s.view->mapFromScene(a))); // la barre est bien dans les limites de la vue
        post(s, QEvent::TabletPress, a, Qt::LeftButton);
        for (int i = 1; i <= 5; ++i) {
            post(s, QEvent::TabletMove, a + QPointF(20.0 * i, 0), Qt::LeftButton);
        }
        post(s, QEvent::TabletRelease, a + QPointF(100, 0), Qt::NoButton);
        QTest::qWait(150);
        QCOMPARE(s.fixed->objectCount(), 0);
    }

    // Page suivante pendant un trait (PgDown, télécommande, doigt sur le bouton) : le trait se termine et
    // reste sur la page où il a été commencé ; rien n'est versé à la nouvelle page, rien n'y reste dessiné.
    void pageChangeDuringAStrokeLeavesItOnItsPage()
    {
        Shell s;
        QVERIFY(start(s));
        auto* ink = s.window->findChild<render::InkItem*>(QStringLiteral("ink"));
        auto* controller = s.window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
        QVERIFY(ink && controller);
        post(s, QEvent::TabletPress, s.view->pageToScene(QPointF(300, 400)), Qt::LeftButton);
        for (int i = 1; i <= 5; ++i) {
            post(s, QEvent::TabletMove, s.view->pageToScene(QPointF(300 + 20.0 * i, 400)), Qt::LeftButton);
        }
        QTRY_COMPARE(ink->pointCount(), std::size_t{6});
        QTest::keyClick(s.window, Qt::Key_PageDown);
        QCOMPARE(indicator(s), QStringLiteral("2 / 2"));
        QCOMPARE(ink->pointCount(), std::size_t{0});
        QCOMPARE(s.fixed->objectCount(), 0);
        for (int i = 6; i <= 10; ++i) {
            post(s, QEvent::TabletMove, s.view->pageToScene(QPointF(300 + 20.0 * i, 400)), Qt::LeftButton);
        }
        post(s, QEvent::TabletRelease, s.view->pageToScene(QPointF(500, 400)), Qt::NoButton);
        QTest::qWait(150);
        QCOMPARE(s.fixed->objectCount(), 0);
        QCOMPARE(ink->pointCount(), std::size_t{0});
        const core::Document& document = controller->document();
        QCOMPARE(document.page(1).objects.size(), std::size_t{0});
        QCOMPARE(document.page(0).objects.size(), std::size_t{1});
        const auto& stroke = std::get<core::Stroke>(document.page(0).objects.front());
        QCOMPARE(stroke.points.size(), std::size_t{6}); // de 300 à 400 : rien d'après le changement de page
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
