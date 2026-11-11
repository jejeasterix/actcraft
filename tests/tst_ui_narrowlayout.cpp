#include "startup.h"
#include "support/wcag.h"

#include <actcraft/ui/layoutmetrics.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/preferences.h>

#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QWheelEvent>
#include <QtQml/qqmlextensionplugin.h>

#include <algorithm>
#include <cmath>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

struct Shell
{
    QTemporaryDir profiles; // profils d'essai, détruit après le moteur
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::PageView* view = nullptr;
};

bool start(Shell& s)
{
    StartupOptions options;
    options.profileRoot = s.profiles.path();
    s.window = loadMainWindow(s.engine, options);
    if (!s.window || !QTest::qWaitForWindowExposed(s.window)) {
        return false;
    }
    s.view = s.window->findChild<ui::PageView*>(QStringLiteral("pageView"));
    return s.view != nullptr;
}

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

// Boutons de commande visibles sous `root` : un CommandButton porte la propriété `commandId`.
void collectCommandButtons(QQuickItem* root, QList<QQuickItem*>& out)
{
    const QList<QQuickItem*> children = root->childItems();
    for (QQuickItem* child : children) {
        if (child->isVisible() && child->property("commandId").isValid()) {
            out.append(child);
        }
        collectCommandButtons(child, out);
    }
}

bool resizeTo(Shell& s, QSize size)
{
    s.window->resize(size);
    if (!QTest::qWaitFor([&] { return s.window->size() == size; }, 5000)) {
        return false;
    }
    QTest::qWait(100); // la ligne se refait au tour de rendu suivant
    return true;
}

void clickCenter(Shell& s, QQuickItem* target)
{
    const QPointF p = target->mapToScene(QPointF(target->width() / 2, target->height() / 2));
    QTest::mouseClick(s.window, Qt::LeftButton, Qt::NoModifier, p.toPoint());
}

} // namespace

class TstUiNarrowlayout : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanup();
    void thresholds();
    void touchUiFromSetting();
    void everyTargetAtLeast48dp();
    void browsersHiddenWhenNarrow();
    void paletteCollapsible();
    void wideLayoutUnchanged();
    void paletteFitsShortWindow();
    void topBarOverflow();
    void overflowLabelContrastWhenOpen();

private:
    QTemporaryDir m_settings;
};

// Réglages d'essai : aucun réglage ni dossier du poste touché.
void TstUiNarrowlayout::initTestCase()
{
    QVERIFY(m_settings.isValid());
    QStandardPaths::setTestModeEnabled(true);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
    QSettings probe;
    QCOMPARE(probe.format(), QSettings::IniFormat);
    QVERIFY(probe.fileName().startsWith(m_settings.path()));
}

void TstUiNarrowlayout::cleanup()
{
    QSettings().remove(QStringLiteral("workstation/touch-ui"));
}

void TstUiNarrowlayout::thresholds()
{
    ui::LayoutMetrics m;
    m.setWindowWidth(959);
    m.setWindowHeight(600);
    QVERIFY(m.narrow());
    QVERIFY(!m.compact());
    QVERIFY(!m.portrait());
    m.setWindowWidth(960);
    QVERIFY(!m.narrow());
    m.setWindowWidth(599);
    QVERIFY(m.narrow());
    QVERIFY(m.compact());
    m.setWindowWidth(600);
    QVERIFY(!m.compact());
    m.setWindowWidth(800);
    m.setWindowHeight(1280);
    QVERIFY(m.portrait());
}

void TstUiNarrowlayout::touchUiFromSetting()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    ui::Preferences preferences(dir.filePath(QStringLiteral("prefs.ini")), nullptr);
    ui::LayoutMetrics m;
    m.setPreferences(&preferences);
#ifndef Q_OS_ANDROID
    QVERIFY(!m.touchUi());
    QCOMPARE(m.targetSize(), 44.0);
    QCOMPARE(m.spacing(), 4.0);
    preferences.setValue(QStringLiteral("workstation/touch-ui"), true);
    QVERIFY(m.touchUi());
    QCOMPARE(m.targetSize(), 48.0);
    QCOMPARE(m.spacing(), 8.0);
    preferences.setValue(QStringLiteral("workstation/touch-ui"), false);
    QVERIFY(!m.touchUi());
    QCOMPARE(m.targetSize(), 44.0);
#else
    QVERIFY(m.touchUi());
    QCOMPARE(m.targetSize(), 48.0);
#endif
}

void TstUiNarrowlayout::everyTargetAtLeast48dp()
{
    {
        QSettings seed;
        seed.setValue(QStringLiteral("workstation/touch-ui"), true);
    }
    Shell s;
    QVERIFY(start(s));
    QVERIFY(resizeTo(s, QSize(800, 1280)));
    auto* layout = s.window->findChild<ui::LayoutMetrics*>(QStringLiteral("appLayout"));
    QVERIFY(layout);
    QTRY_VERIFY(layout->touchUi());
    // Palette et tiroir ouverts : leurs boutons sont alors visibles.
    QQuickItem* toggle = item(s, "paletteToggle");
    QVERIFY(toggle);
    clickCenter(s, toggle);
    QTRY_VERIFY(toggle->property("checked").toBool()); // la palette est bien ouverte
    QQuickItem* browser = item(s, "browserPanel");
    QVERIFY(browser);
    QVERIFY(QMetaObject::invokeMethod(browser, "toggleDrawer"));
    QTRY_VERIFY(browser->isVisible());

    QList<QQuickItem*> buttons;
    collectCommandButtons(s.window->contentItem(), buttons);
    QVERIFY(buttons.size() > 10);
    for (QQuickItem* b : std::as_const(buttons)) {
        QVERIFY2(
            b->width() >= 48 && b->height() >= 48,
            qPrintable(QStringLiteral("%1 : %2 x %3").arg(b->objectName()).arg(b->width()).arg(b->height())));
    }
    // Deux voisins du même parent : au moins 8 dp d'écart.
    for (qsizetype i = 0; i < buttons.size(); ++i) {
        for (qsizetype j = i + 1; j < buttons.size(); ++j) {
            QQuickItem* a = buttons.at(i);
            QQuickItem* b = buttons.at(j);
            if (a->parentItem() != b->parentItem()) {
                continue;
            }
            const QRectF ra(a->position(), a->size());
            const QRectF rb(b->position(), b->size());
            const qreal gx = std::max(ra.left(), rb.left()) - std::min(ra.right(), rb.right());
            const qreal gy = std::max(ra.top(), rb.top()) - std::min(ra.bottom(), rb.bottom());
            QVERIFY2(
                std::max(gx, gy) >= 8.0 - 0.01,
                qPrintable(QStringLiteral("%1 et %2 trop proches").arg(a->objectName(), b->objectName())));
        }
    }
}

void TstUiNarrowlayout::browsersHiddenWhenNarrow()
{
    Shell s;
    QVERIFY(start(s));
    QQuickItem* browser = item(s, "browserPanel");
    QQuickItem* pageArea = item(s, "pageArea");
    QVERIFY(browser && pageArea);
    // Large : le panneau est dans la ligne (il pousse la page).
    const qreal closedWidth = pageArea->width();
    QVERIFY(QMetaObject::invokeMethod(browser, "toggleDrawer"));
    QTRY_VERIFY(browser->isVisible());
    QTRY_VERIFY(pageArea->width() < closedWidth);
    QVERIFY(QMetaObject::invokeMethod(browser, "toggleDrawer"));
    QTRY_VERIFY(!browser->isVisible());
    QTRY_COMPARE(pageArea->width(), closedWidth);

    QVERIFY(resizeTo(s, QSize(800, 1280)));
    QVERIFY(!browser->isVisible());
    QVERIFY(!browser->property("opened").toBool());
    const qreal narrowWidth = pageArea->width();
    // Le bouton ouvre le tiroir : il recouvre la page sans la pousser.
    QQuickItem* button = item(s, "button-view.browsers");
    QVERIFY(button && button->isVisible());
    clickCenter(s, button);
    QTRY_VERIFY(browser->property("opened").toBool());
    QVERIFY(browser->isVisible());
    QCOMPARE(pageArea->width(), narrowWidth);
    QVERIFY(browser->width() > 0);
    // Appui hors du tiroir : il se referme.
    const QPointF away = pageArea->mapToScene(QPointF(pageArea->width() - 20, pageArea->height() / 2));
    QTest::mouseClick(s.window, Qt::LeftButton, Qt::NoModifier, away.toPoint());
    QTRY_VERIFY(!browser->property("opened").toBool());
    QVERIFY(!browser->isVisible());
    // Retour en large : le panneau reprend sa place dans la ligne.
    QVERIFY(resizeTo(s, QSize(1280, 800)));
    QVERIFY(QMetaObject::invokeMethod(browser, "toggleDrawer"));
    QTRY_VERIFY(browser->isVisible());
    QTRY_VERIFY(pageArea->width() < closedWidth);
}

void TstUiNarrowlayout::paletteCollapsible()
{
    Shell s;
    QVERIFY(start(s));
    QQuickItem* toggle = item(s, "paletteToggle");
    QQuickItem* panel = item(s, "palettePanel");
    QVERIFY(toggle && panel);
    QVERIFY(!toggle->isVisible()); // large : pas de bouton de repli
    QVERIFY(panel->isVisible());

    QVERIFY(resizeTo(s, QSize(800, 1280)));
    QTRY_VERIFY(toggle->isVisible());
    QVERIFY(!panel->isVisible()); // repliée en un bouton
    QVERIFY(toggle->width() >= 44);
    clickCenter(s, toggle);
    QTRY_VERIFY(panel->isVisible());
    clickCenter(s, toggle);
    QTRY_VERIFY(!panel->isVisible());

    QVERIFY(resizeTo(s, QSize(1280, 800)));
    QTRY_VERIFY(panel->isVisible());
    QVERIFY(!toggle->isVisible());
}

void TstUiNarrowlayout::wideLayoutUnchanged()
{
    Shell s;
    QVERIFY(start(s));
    QVERIFY(resizeTo(s, QSize(1280, 800)));
    QQuickItem* palette = item(s, "toolPalette");
    QVERIFY(palette);
    QList<QQuickItem*> buttons;
    collectCommandButtons(palette, buttons);
    QStringList got;
    for (QQuickItem* b : std::as_const(buttons)) {
        const QPointF p = b->mapToScene(QPointF(0, 0));
        got << QStringLiteral("%1 %2,%3 %4x%5")
                   .arg(b->objectName())
                   .arg(std::lround(p.x()))
                   .arg(std::lround(p.y()))
                   .arg(std::lround(b->width()))
                   .arg(std::lround(b->height()));
    }
    got.prepend(QStringLiteral("palette %1,%2 %3x%4")
                    .arg(std::lround(palette->mapToScene(QPointF(0, 0)).x()))
                    .arg(std::lround(palette->mapToScene(QPointF(0, 0)).y()))
                    .arg(std::lround(palette->width()))
                    .arg(std::lround(palette->height())));
    // Positions relevées avant la tâche (fenêtre 1280 x 800, sans touch-ui).
    QStringList expected = {QStringLiteral("palette 1168,72 112x728"),
                            QStringLiteral("button-menu.main 1176,80 44x44"),
                            QStringLiteral("button-menu.tools 1224,80 44x44"),
                            QStringLiteral("button-profile.choose 1176,128 44x44"),
                            QStringLiteral("button-mode.design 1224,128 44x44"),
                            QStringLiteral("button-page.previous 1176,189 44x44"),
                            QStringLiteral("button-page.next 1224,189 44x44"),
                            QStringLiteral("button-page.history-back 1176,237 44x44"),
                            QStringLiteral("button-page.history-forward 1224,237 44x44"),
                            QStringLiteral("button-tool.select 1176,536 44x44"),
                            QStringLiteral("button-tool.pen 1224,536 44x44"),
                            QStringLiteral("button-tool.highlighter 1176,584 44x44"),
                            QStringLiteral("button-tool.reveal-ink 1224,584 44x44"),
                            QStringLiteral("button-tool.eraser 1176,632 44x44"),
                            QStringLiteral("button-edit.undo 1176,693 44x44"),
                            QStringLiteral("button-edit.redo 1224,693 44x44"),
                            QStringLiteral("button-edit.delete 1176,741 44x44"),
                            QStringLiteral("button-present.curtain 1176,802 44x44"),
                            QStringLiteral("button-present.spotlight-circle 1224,802 44x44"),
                            QStringLiteral("button-view.page-zoom 1176,850 44x44"),
                            QStringLiteral("button-math.ruler 1224,850 44x44"),
                            QStringLiteral("button-tool.clock 1176,898 44x44"),
                            QStringLiteral("button-tool.shape 1176,959 44x44"),
                            QStringLiteral("button-tool.text 1224,959 44x44"),
                            QStringLiteral("button-tool.fill 1176,1007 44x44"),
                            QStringLiteral("button-tool.connector 1224,1007 44x44"),
                            QStringLiteral("trashCan 1168,740 112x60")};
#ifdef Q_OS_WIN
    // launcher.edit n'existe que sous Windows : son bouton ferme la barre de raccourcis, après les groupes.
    expected.insert(expected.size() - 1, QStringLiteral("launcherEditButton 1176,1068 44x44"));
#endif
    if (got != expected) {
        qDebug().noquote() << "RELEVE" << got.join(QStringLiteral("\nRELEVE "));
    }
    QCOMPARE(got, expected);
}

// Amendement 3, volet 2 : palette entière dans une fenêtre de 1 000 px de haut ou moins.
void TstUiNarrowlayout::paletteFitsShortWindow()
{
    Shell s;
    QVERIFY(start(s));
    for (const QSize size : {QSize(1280, 1000), QSize(1280, 720)}) {
        QVERIFY(resizeTo(s, size));
        QQuickItem* flick = item(s, "paletteFlick");
        QQuickItem* bar = item(s, "paletteScrollBar");
        QQuickItem* rows = item(s, "paletteRows");
        QVERIFY(flick && bar && rows);
        flick->setProperty("contentY", 0.0);
        QTRY_VERIFY(flick->height() > 0);

        QList<QQuickItem*> buttons;
        collectCommandButtons(rows, buttons);
        QVERIFY(!buttons.isEmpty());
        const auto bottomIn = [rows](QQuickItem* b) {
            return b->mapToItem(rows, QPointF(0, b->height())).y();
        };
        QQuickItem* last =
            *std::max_element(buttons.cbegin(), buttons.cend(),
                              [&](QQuickItem* a, QQuickItem* b) { return bottomIn(a) < bottomIn(b); });
        const auto inView = [flick, last] {
            const qreal top = last->mapToItem(flick, QPointF(0, 0)).y();
            const qreal bottom = last->mapToItem(flick, QPointF(0, last->height())).y();
            return top >= -0.5 && bottom <= flick->height() + 0.5;
        };

        const bool overflows = flick->property("contentHeight").toReal() > flick->height();
        if (size.height() == 720) {
            QVERIFY2(overflows,
                     "à 720 px de haut la palette doit déborder, sinon la fonction ne prouve rien");
            QVERIFY(!inView());
        }
        if (!overflows) {
            QVERIFY(!bar->isVisible());
            QVERIFY(inView());
            continue;
        }
        // Débordement : la poignée se voit, elle ne recouvre aucun bouton, la molette amène le dernier
        // bouton.
        QVERIFY(bar->isVisible());
        QCOMPARE(bar->property("policy").toInt(), 2); // ScrollBar.AlwaysOn
        auto* thumb = bar->property("contentItem").value<QQuickItem*>();
        QVERIFY(thumb && thumb->opacity() > 0.5);
        const QRectF barRect = bar->mapRectToItem(flick, QRectF(0, 0, bar->width(), bar->height()));
        for (QQuickItem* b : std::as_const(buttons)) {
            QVERIFY(!b->mapRectToItem(flick, QRectF(0, 0, b->width(), b->height())).intersects(barRect));
        }
        const QPointF at = flick->mapToScene(QPointF(flick->width() / 2, flick->height() / 2));
        for (int i = 0; i < 40 && !inView(); ++i) {
            QWheelEvent wheel(at, s.window->mapToGlobal(at), QPoint(), QPoint(0, -120), Qt::NoButton,
                              Qt::NoModifier, Qt::NoScrollPhase, false);
            wheel.setTimestamp(static_cast<ulong>(1000 * (i + 1)));
            QCoreApplication::sendEvent(s.window, &wheel);
            QTest::qWait(20);
        }
        QTRY_VERIFY(inView());
    }
}

void TstUiNarrowlayout::topBarOverflow()
{
    Shell s;
    QVERIFY(start(s));
    QQuickItem* topBar = item(s, "topBar");
    QVERIFY(topBar);
    // Le débordement appartient à la barre : le bouton « … » en est un descendant.
    QQuickItem* overflow = findItem(topBar, QStringLiteral("topOverflow"));
    QVERIFY(overflow);
    QQuickItem* menu = findItem(topBar, QStringLiteral("menu-file"));
    QVERIFY(menu);
    // Large : menus visibles, bouton caché.
    QVERIFY(resizeTo(s, QSize(1280, 800)));
    QVERIFY(menu->isVisible());
    QVERIFY(!overflow->isVisible());
    // Compacte (< 600 dp) : menus cachés, bouton visible, son clic ouvre le menu principal.
    QVERIFY(resizeTo(s, QSize(500, 900)));
    QTRY_VERIFY(!menu->isVisible());
    QVERIFY(overflow->isVisible());
    QObject* mainMenu = s.window->findChild<QObject*>(QStringLiteral("mainMenu"));
    QVERIFY(mainMenu);
    QVERIFY(!mainMenu->property("opened").toBool());
    clickCenter(s, overflow);
    QTRY_VERIFY(mainMenu->property("opened").toBool());
    QVERIFY(QMetaObject::invokeMethod(mainMenu, "close"));
    QTRY_VERIFY(!mainMenu->property("opened").toBool());
    // Retour en large.
    QVERIFY(resizeTo(s, QSize(1280, 800)));
    QTRY_VERIFY(menu->isVisible());
    QVERIFY(!overflow->isVisible());
}

// L11-C17 : le libellé du bouton « … » ouvert reste lisible (≥ 4,5:1) sur son fond actif.
void TstUiNarrowlayout::overflowLabelContrastWhenOpen()
{
    Shell s;
    QVERIFY(start(s));
    QVERIFY(resizeTo(s, QSize(500, 900)));
    QQuickItem* topBar = item(s, "topBar");
    QVERIFY(topBar);
    QQuickItem* overflow = findItem(topBar, QStringLiteral("topOverflow"));
    QVERIFY(overflow);
    QTRY_VERIFY(overflow->isVisible());
    QObject* mainMenu = s.window->findChild<QObject*>(QStringLiteral("mainMenu"));
    QVERIFY(mainMenu);
    clickCenter(s, overflow);
    QTRY_VERIFY(mainMenu->property("opened").toBool());
    // contentItem (Label) et fond (Rectangle) du bouton : les couleurs réellement affichées.
    QObject* label = overflow->property("contentItem").value<QObject*>();
    QObject* background = overflow->property("background").value<QObject*>();
    QVERIFY(label && background);
    const QColor text = label->property("color").value<QColor>();
    const QColor fill = background->property("color").value<QColor>();
    QVERIFY(text.isValid() && fill.isValid() && fill.alpha() == 255);
    const double ratio = wcag::contrastRatio(text, fill);
    QVERIFY2(ratio >= wcag::kText, qPrintable(QStringLiteral("contraste %1").arg(ratio)));
    QVERIFY(QMetaObject::invokeMethod(mainMenu, "close"));
}

QTEST_MAIN(TstUiNarrowlayout)
#include "tst_ui_narrowlayout.moc"
