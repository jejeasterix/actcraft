#include "startup.h"

#include <actcraft/core/document.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>

#include <QImage>
#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStringList>
#include <QTabletEvent>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <algorithm>
#include <cmath>
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

// ---- Contrastes réellement rendus (WCAG 2.1 AA, L1-C5) ----
// Les couleurs sont lues sur les éléments affichés (nos composants comme ceux du style Basic, que la palette
// de la fenêtre colore), dans chaque état ; aucune paire n'est recopiée à la main.

double relativeLuminance(const QColor& color)
{
    const auto linear = [](double c) {
        return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) + 0.0722 * linear(color.blueF());
}

double contrastRatio(const QColor& a, const QColor& b)
{
    const double la = relativeLuminance(a);
    const double lb = relativeLuminance(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

constexpr double kText = 4.5;
constexpr double kUi = 3.0;

QColor colorProperty(const QObject* object, const char* name)
{
    return object ? object->property(name).value<QColor>() : QColor();
}

QQuickItem* itemProperty(const QObject* object, const char* name)
{
    return object ? object->property(name).value<QQuickItem*>() : nullptr;
}

bool opaqueRectangle(const QQuickItem* item)
{
    return item && item->isVisible() && item->inherits("QQuickRectangle") &&
           colorProperty(item, "color").alpha() == 255;
}

// Fond réellement sous un élément : en remontant les parents, le premier fond opaque visible (le `background`
// d'un contrôle, ou un Rectangle opaque) ; la couleur de la fenêtre sinon.
QColor backgroundUnder(QQuickItem* item, QQuickWindow* window)
{
    for (QQuickItem* it = item; it; it = it->parentItem()) {
        QQuickItem* background = itemProperty(it, "background");
        if (background != item && opaqueRectangle(background)) {
            return colorProperty(background, "color");
        }
        if (it != item && opaqueRectangle(it)) {
            return colorProperty(it, "color");
        }
    }
    return window->color();
}

double effectiveOpacity(const QQuickItem* item)
{
    double opacity = 1.0;
    for (const QQuickItem* it = item; it; it = it->parentItem()) {
        opacity *= it->opacity();
    }
    return opacity;
}

QColor over(const QColor& fg, const QColor& bg, double alpha)
{
    return QColor::fromRgbF(static_cast<float>(fg.redF() * alpha + bg.redF() * (1 - alpha)),
                            static_cast<float>(fg.greenF() * alpha + bg.greenF() * (1 - alpha)),
                            static_cast<float>(fg.blueF() * alpha + bg.blueF() * (1 - alpha)));
}

QString describe(const QQuickItem* item)
{
    QStringList path;
    for (const QQuickItem* it = item; it && path.size() < 6; it = it->parentItem()) {
        path << (it->objectName().isEmpty() ? QString::fromLatin1(it->metaObject()->className())
                                            : it->objectName());
    }
    return path.join(QStringLiteral(" < "));
}

void checkPair(QStringList& failures, const QString& what, const QQuickItem* item, const QColor& fg,
               const QColor& bg, double minimum)
{
    const double ratio = contrastRatio(fg, bg);
    if (ratio < minimum) {
        failures << QStringLiteral("%1 %2 sur %3 : %4:1 < %5:1 (%6)")
                        .arg(what, fg.name(), bg.name())
                        .arg(ratio, 0, 'f', 2)
                        .arg(minimum, 0, 'f', 1)
                        .arg(describe(item));
    }
}

// Disponibilité du contrôle qui contient l'élément (le champ de texte du zoom est désactivé par le style pour
// ne pas être éditable, mais il est affiché comme un texte actif).
bool controlEnabled(const QQuickItem* item)
{
    for (const QQuickItem* it = item; it; it = it->parentItem()) {
        if (it->inherits("QQuickControl")) {
            return it->isEnabled();
        }
    }
    return true;
}

// Tout texte visible d'une commande disponible (4,5:1, lien compris) et toute icône teintée (3:1) contre le
// fond réellement dessiné sous lui. Les éléments d'une commande indisponible sont exemptés par WCAG.
void collectRenderedPairs(QQuickItem* item, QQuickWindow* window, QStringList& failures)
{
    if (!item || !item->isVisible() || effectiveOpacity(item) < 0.01) {
        return;
    }
    const bool text =
        item->inherits("QQuickText") || item->inherits("QQuickTextEdit") || item->inherits("QQuickTextInput");
    const bool icon = item->inherits("QQuickIconImage") || item->inherits("QQuickColorImage");
    if (controlEnabled(item) && (text || icon)) {
        const QColor bg = backgroundUnder(item, window);
        const QColor fg = over(colorProperty(item, "color"), bg, effectiveOpacity(item));
        if (text && !item->property("text").toString().isEmpty()) {
            checkPair(failures, QStringLiteral("texte"), item, fg, bg, kText);
            if (item->property("text").toString().contains(QLatin1String("<a "))) {
                checkPair(failures, QStringLiteral("lien"), item, colorProperty(item, "linkColor"), bg,
                          kText);
            }
        }
        if (icon && colorProperty(item, "color").alpha() > 0 && !item->property("source").toUrl().isEmpty()) {
            checkPair(failures, QStringLiteral("icône"), item, fg, bg, kUi);
        }
    }
    const QList<QQuickItem*> children = item->childItems();
    for (QQuickItem* child : children) {
        collectRenderedPairs(child, window, failures);
    }
}

QStringList renderedFailures(Shell& s)
{
    QStringList failures;
    collectRenderedPairs(s.window->contentItem(), s.window, failures);
    return failures;
}

// Un état (entrée surlignée, contrôle appuyé) doit se voir contre ce qui l'entoure : son fond, ou son bord
// d'au moins 2 px, à 3:1 au moins du fond du conteneur (WCAG 1.4.11).
void checkStateIndicator(QStringList& failures, const QString& what, QQuickItem* control,
                         const QColor& around)
{
    QQuickItem* background = itemProperty(control, "background");
    double best = 0.0;
    if (background && background->isVisible()) {
        const QColor fill = colorProperty(background, "color");
        if (fill.alpha() == 255) {
            best = contrastRatio(fill, around);
        }
        QObject* border = background->property("border").value<QObject*>();
        if (border && border->property("width").toDouble() >= 2.0) {
            best = std::max(best, contrastRatio(colorProperty(border, "color"), around));
        }
    }
    if (best < kUi) {
        failures << QStringLiteral("%1 : indicateur à %2:1 contre %3 (%4)")
                        .arg(what)
                        .arg(best, 0, 'f', 2)
                        .arg(around.name(), describe(control));
    }
}

// Nombre de pixels d'une zone dont la couleur change d'au moins 3:1 entre deux images (indicateur de focus).
int pixelsChangedBy3(const QImage& before, const QImage& after, const QRect& zone)
{
    int count = 0;
    const QRect area = zone.intersected(before.rect());
    for (int y = area.top(); y <= area.bottom(); ++y) {
        for (int x = area.left(); x <= area.right(); ++x) {
            if (contrastRatio(before.pixelColor(x, y), after.pixelColor(x, y)) >= kUi) {
                ++count;
            }
        }
    }
    return count;
}

QColor themeColor(Shell& s, const char* token)
{
    QQmlComponent component(&s.engine);
    component.setData("import QtQuick\nimport ActCraft.Ui\nQtObject { property QtObject theme: Theme }\n",
                      QUrl());
    std::unique_ptr<QObject> holder(component.create());
    return holder ? colorProperty(holder->property("theme").value<QObject*>(), token) : QColor();
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
              "menu-file", "menu-edit", "menu-view", "menu-insert", "menu-tools", "menu-help",
              "button-page.next", "button-page.previous", "button-tool.pen", "button-menu.main",
              "button-menu.tools", "button-view.fullscreen"}) {
            QQuickItem* found = item(s, name);
            QVERIFY2(found && found->isVisible(), name);
        }
        // Éditer et Insérer, vides au lot 1, sont garnis par le socle des lots 3 et 7 (L0-S4) : ils
        // s'affichent.
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
        QCOMPARE(s.view->inkColor(), QColor(QStringLiteral("#E53935")));
        QVERIFY(run(s, QStringLiteral("pen.width"), 8));
        QCOMPARE(s.view->inkWidth(), 8.0);
        QVERIFY(!run(s, QStringLiteral("pen.color"), QStringLiteral("rouge")));
        QCOMPARE(s.view->inkColor(), QColor(QStringLiteral("#E53935")));
    }

    void widthSliderFollowsTheInkWidth()
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
        QTRY_COMPARE(s.view->inkWidth(), 8.0);
        QTRY_VERIFY(slider->property("visualPosition").toDouble() > 0.3);
        // Sens inverse : déplacer la réglette change l'épaisseur du stylo.
        const QPointF origin = slider->mapToScene(QPointF(0, 0));
        const QPoint mid = (origin + QPointF(slider->width() / 2, slider->height() / 2)).toPoint();
        QTest::mouseClick(s.window, Qt::LeftButton, Qt::NoModifier, mid);
        QTRY_VERIFY2(s.view->inkWidth() > 8.0 && s.view->inkWidth() < 100.0,
                     qPrintable(QString::number(s.view->inkWidth())));
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

    // L'état réel de la fenêtre (visibility), pas une propriété affectée avant de le demander.
    void f5TogglesFullScreen()
    {
        Shell s;
        QVERIFY(start(s));
        auto* commands = s.window->findChild<ui::Commands*>(QStringLiteral("commands"));
        QVERIFY(commands);
        QVERIFY(s.window->visibility() != QWindow::FullScreen);
        QTest::keyClick(s.window, Qt::Key_F5);
        QTRY_COMPARE(s.window->visibility(), QWindow::FullScreen);
        QVERIFY(s.window->property("fullScreen").toBool());
        QVERIFY(commands->isChecked(QStringLiteral("view.fullscreen")));
        QTest::keyClick(s.window, Qt::Key_F5);
        QTRY_VERIFY(s.window->visibility() == QWindow::Windowed ||
                    s.window->visibility() == QWindow::Maximized);
        QVERIFY(!s.window->property("fullScreen").toBool());
        QVERIFY(!commands->isChecked(QStringLiteral("view.fullscreen")));
    }

    // Sortie du plein écran par le système (raccourci du gestionnaire de fenêtres, autre application) : la
    // coche suit, et une seule pression sur F5 y revient.
    void fullScreenLeftByTheSystemFollowsTheRealState()
    {
        Shell s;
        QVERIFY(start(s));
        auto* commands = s.window->findChild<ui::Commands*>(QStringLiteral("commands"));
        QVERIFY(commands);
        QTest::keyClick(s.window, Qt::Key_F5);
        QTRY_COMPARE(s.window->visibility(), QWindow::FullScreen);
        s.window->showNormal(); // le système, pas la commande
        QTRY_VERIFY(!s.window->property("fullScreen").toBool());
        QVERIFY(!commands->isChecked(QStringLiteral("view.fullscreen")));
        QTest::keyClick(s.window, Qt::Key_F5);
        QTRY_COMPARE(s.window->visibility(), QWindow::FullScreen);
        QVERIFY(commands->isChecked(QStringLiteral("view.fullscreen")));
    }

    void swatchTapChoosesTheInkColor()
    {
        Shell s;
        QVERIFY(start(s));
        QQuickItem* swatch = item(s, "swatch-5");
        QVERIFY(swatch);
        const QPointF center = swatch->mapToScene(QPointF(swatch->width() / 2, swatch->height() / 2));
        QTest::mouseClick(s.window, Qt::LeftButton, Qt::NoModifier, center.toPoint());
        QTRY_COMPARE(s.view->inkColor(), QColor(QStringLiteral("#1E88E5")));
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

    // L1-C5 : chaque texte, lien et icône réellement rendu tient le seuil AA contre le fond dessiné sous lui,
    // dans chaque état : repos, appuyé (au doigt, sans survol), menus et liste du zoom ouverts avec une
    // entrée surlignée (indicateur à 3:1 du menu), dialogue « À propos ».
    void renderedColorsMeetWcagAa()
    {
        Shell s;
        QVERIFY(start(s));
        QStringList failures = renderedFailures(s);

        QQuickItem* next = item(s, "button-page.next");
        QQuickItem* fileMenu = item(s, "menu-file");
        QVERIFY(next && fileMenu);
        QVERIFY(next->setProperty("down", true));
        QVERIFY(fileMenu->setProperty("down", true));
        failures << renderedFailures(s);
        checkStateIndicator(failures, QStringLiteral("bouton appuyé"), next,
                            colorProperty(item(s, "toolPalette"), "color"));
        checkStateIndicator(failures, QStringLiteral("menu de la barre appuyé"), fileMenu,
                            colorProperty(item(s, "topBar"), "color"));
        next->setProperty("down", QVariant());
        fileMenu->setProperty("down", QVariant());

        const struct
        {
            const char* command;
            const char* menu;
        } menus[] = {{"menu.tools", "toolsMenu"}, {"menu.main", "mainMenu"}};
        for (const auto& m : menus) {
            QVERIFY(run(s, QString::fromLatin1(m.command)));
            QObject* menu = s.window->findChild<QObject*>(QString::fromLatin1(m.menu));
            QVERIFY(menu);
            QTRY_VERIFY(menu->property("opened").toBool());
            QVERIFY(menu->setProperty("currentIndex", 0));
            QQuickItem* entry = nullptr;
            QVERIFY(
                QMetaObject::invokeMethod(menu, "itemAt", Q_RETURN_ARG(QQuickItem*, entry), Q_ARG(int, 0)));
            QVERIFY(entry);
            QTRY_VERIFY(entry->property("highlighted").toBool());
            failures << renderedFailures(s);
            checkStateIndicator(failures, QStringLiteral("entrée surlignée de %1").arg(QLatin1String(m.menu)),
                                entry, colorProperty(itemProperty(menu, "background"), "color"));
            QVERIFY(QMetaObject::invokeMethod(menu, "close"));
            QTRY_VERIFY(!menu->property("visible").toBool());
        }

        QQuickItem* zoomBox = item(s, "zoomBox");
        QVERIFY(zoomBox);
        QObject* popup = zoomBox->property("popup").value<QObject*>();
        QVERIFY(popup);
        QVERIFY(QMetaObject::invokeMethod(popup, "open"));
        QTRY_VERIFY(popup->property("opened").toBool());
        QVERIFY(QMetaObject::invokeMethod(zoomBox, "incrementCurrentIndex"));
        const int highlighted = zoomBox->property("highlightedIndex").toInt();
        QVERIFY(highlighted >= 0);
        QQuickItem* list = itemProperty(popup, "contentItem");
        QQuickItem* entry = nullptr;
        QVERIFY(QMetaObject::invokeMethod(list, "itemAtIndex", Q_RETURN_ARG(QQuickItem*, entry),
                                          Q_ARG(int, highlighted)));
        QVERIFY(entry);
        QTRY_VERIFY(entry->property("highlighted").toBool());
        failures << renderedFailures(s);
        checkStateIndicator(failures, QStringLiteral("entrée surlignée du zoom"), entry,
                            colorProperty(itemProperty(popup, "background"), "color"));
        QVERIFY(QMetaObject::invokeMethod(popup, "close"));
        QTRY_VERIFY(!popup->property("visible").toBool());

        // Sous un dialogue modal, le reste de la fenêtre est assombri et inactif : seul le dialogue compte.
        QVERIFY(run(s, QStringLiteral("help.about")));
        QObject* dialog = s.window->findChild<QObject*>(QStringLiteral("aboutDialog"));
        QVERIFY(dialog);
        QTRY_VERIFY(dialog->property("opened").toBool());
        QQuickItem* dialogContent = itemProperty(dialog, "contentItem");
        QVERIFY(dialogContent && dialogContent->parentItem());
        collectRenderedPairs(dialogContent->parentItem(), s.window, failures);
        // Pouce des notices, appuyé, sur leur puits couleur de page.
        QQuickItem* aboutBar = findItem(dialogContent->parentItem(), QStringLiteral("aboutScrollBar"));
        QVERIFY(aboutBar);
        QVERIFY(aboutBar->setProperty("pressed", true));
        checkPair(failures, QStringLiteral("pouce des notices"), aboutBar,
                  colorProperty(itemProperty(aboutBar, "contentItem"), "color"), themeColor(s, "page"), kUi);
        aboutBar->setProperty("pressed", false);
        QVERIFY(QMetaObject::invokeMethod(dialog, "close"));

        QVERIFY2(failures.isEmpty(), qPrintable(failures.join(QLatin1Char('\n'))));
    }

    // L1-C5, WCAG 2.4.7 : le focus clavier se voit. Au moins 30 pixels autour du contrôle changent de 3:1
    // quand il le reçoit par Tab.
    void keyboardFocusIsVisible_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("part"); // élément dont la zone est observée (vide : le contrôle)
        QTest::newRow("réglette") << QStringLiteral("widthSlider") << QStringLiteral("handle");
        QTest::newRow("menu de la barre") << QStringLiteral("menu-file") << QString();
        QTest::newRow("zoom") << QStringLiteral("zoomBox") << QString();
    }

    void keyboardFocusIsVisible()
    {
        QFETCH(QString, name);
        QFETCH(QString, part);
        Shell s;
        QVERIFY(start(s));
        s.window->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(s.window));
        QQuickItem* control = findItem(s.window->contentItem(), name);
        QVERIFY(control);
        QQuickItem* observed = part.isEmpty() ? control : itemProperty(control, qPrintable(part));
        QVERIFY(observed);
        const QImage before = s.window->grabWindow();
        control->forceActiveFocus(Qt::TabFocusReason);
        QTRY_VERIFY(control->property("visualFocus").toBool());
        const QImage after = s.window->grabWindow();
        const qreal dpr = s.window->effectiveDevicePixelRatio();
        const QRectF scene = observed->mapRectToScene(observed->boundingRect()).adjusted(-6, -6, 6, 6);
        const QRect zone = QRectF(scene.topLeft() * dpr, scene.size() * dpr).toAlignedRect();
        const int changed = pixelsChangedBy3(before, after, zone);
        QVERIFY2(changed >= 30, qPrintable(QStringLiteral("%1 pixels changés de 3:1").arg(changed)));
    }

    // L1-C5 : le pouce des barres de défilement reste lisible, au repos comme appuyé, sur la page comme sur
    // le fond (son remplissage ou son bord à 3:1 au moins).
    void scrollThumbIsVisibleOnPageAndBackground()
    {
        Shell s;
        QVERIFY(start(s));
        QVERIFY(run(s, QStringLiteral("view.zoom"), 200));
        QVERIFY(ready(s));
        QQuickItem* bar = item(s, "verticalScrollBar");
        QVERIFY(bar);
        QTRY_VERIFY(bar->isVisible());
        const QColor page = themeColor(s, "page");
        QVERIFY(page.isValid());
        QStringList failures;
        for (const bool pressed : {false, true}) {
            QVERIFY(bar->setProperty("pressed", pressed));
            QQuickItem* thumb = itemProperty(bar, "contentItem");
            QVERIFY(thumb);
            QObject* border = thumb->property("border").value<QObject*>();
            for (const QColor& around : {page, s.window->color()}) {
                double best = contrastRatio(colorProperty(thumb, "color"), around);
                if (border && border->property("width").toDouble() >= 1.0) {
                    best = std::max(best, contrastRatio(colorProperty(border, "color"), around));
                }
                if (best < kUi) {
                    failures << QStringLiteral("pouce %1 sur %2 : %3:1")
                                    .arg(pressed ? QStringLiteral("appuyé") : QStringLiteral("au repos"),
                                         around.name())
                                    .arg(best, 0, 'f', 2);
                }
            }
        }
        QVERIFY2(failures.isEmpty(), qPrintable(failures.join(QLatin1Char('\n'))));
    }
};

QTEST_MAIN(TstUiShell)
#include "tst_ui_shell.moc"
