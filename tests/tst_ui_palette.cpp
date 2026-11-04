#include "startup.h"

#include <actcraft/ui/commands.h>
#include <actcraft/ui/profilecontroller.h>

#include <QQmlApplicationEngine>
#include <QQmlProperty>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

// Montage de tst_ui_shell : l'interface réelle, avec des profils d'essai propres au montage (amendement 3, N1).
struct Shell
{
    QTemporaryDir profiles; // détruit après le moteur
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
};

bool start(Shell& s)
{
    StartupOptions options;
    options.profileRoot = s.profiles.path();
    s.window = loadMainWindow(s.engine, options);
    return s.window && QTest::qWaitForWindowExposed(s.window);
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

void collectButtons(QQuickItem* root, QList<QQuickItem*>& out)
{
    if (root->objectName().startsWith(QLatin1String("button-"))) {
        out.append(root);
    }
    const QList<QQuickItem*> children = root->childItems();
    for (QQuickItem* child : children) {
        collectButtons(child, out);
    }
}

ui::ProfileController* profileOf(Shell& s)
{
    return qobject_cast<ui::ProfileController*>(s.window->findChild<QObject*>(QStringLiteral("appProfile")));
}

ui::Commands* commandsOf(Shell& s)
{
    return qobject_cast<ui::Commands*>(s.window->findChild<QObject*>(QStringLiteral("commands")));
}

QPointF sceneOf(QQuickItem* target, QPointF local)
{
    return target->mapToScene(local);
}

} // namespace

class TstUiPalette : public QObject
{
    Q_OBJECT
    QTemporaryDir m_settingsDir; // réglages d'essai : jamais ceux du poste (amendement 3, N1)

private slots:
    void initTestCase()
    {
        QVERIFY(m_settingsDir.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
    }

    void dockFollowsTheProfile()
    {
        Shell s;
        QVERIFY(start(s));
        auto* palette = item(s, "toolPalette");
        QVERIFY(palette);
        auto* profile = profileOf(s);
        QVERIFY(profile);
        profile->setPaletteDock(QStringLiteral("left"));
        QTRY_COMPARE(sceneOf(palette, QPointF(0, 0)).x(), 0.0);
        profile->setPaletteDock(QStringLiteral("right"));
        QTRY_COMPARE(sceneOf(palette, QPointF(palette->width(), 0)).x(), double(s.window->width()));
        profile->setPaletteDock(QStringLiteral("top"));
        QTRY_COMPARE(sceneOf(palette, QPointF(0, 0)).y(), sceneOf(item(s, "paletteFrame")->parentItem(), QPointF(0, 0)).y() + item(s, "paletteTitle")->height());
        profile->setPaletteDock(QStringLiteral("bottom"));
        QTRY_COMPARE(sceneOf(palette, QPointF(0, palette->height())).y(), double(s.window->height()));
    }

    void dragToCenterFloats()
    {
        Shell s;
        QVERIFY(start(s));
        auto* title = item(s, "paletteTitle");
        QVERIFY(title);
        const QPoint from = sceneOf(title, QPointF(title->width() / 2, title->height() / 2)).toPoint();
        const QPoint to(s.window->width() / 2, s.window->height() / 2);
        QTest::mousePress(s.window, Qt::LeftButton, {}, from);
        QTest::mouseMove(s.window, from + QPoint(10, 10));
        QTest::mouseMove(s.window, to);
        QTest::mouseRelease(s.window, Qt::LeftButton, {}, to);
        QTRY_COMPARE(profileOf(s)->paletteDock(), QStringLiteral("floating"));
        QVERIFY(profileOf(s)->paletteFloat().x() > 100);
    }

    void dragToEdgeDocks()
    {
        Shell s;
        QVERIFY(start(s));
        auto* profile = profileOf(s);
        profile->setPaletteDock(QStringLiteral("floating"));
        profile->setPaletteFloat(QPointF(400, 200));
        auto* title = item(s, "paletteTitle");
        QVERIFY(title);
        QTRY_COMPARE(sceneOf(item(s, "palettePanel"), QPointF(0, 0)).x(), 400.0);
        const QPoint from = sceneOf(title, QPointF(title->width() / 2, title->height() / 2)).toPoint();
        const QPoint to(from.x() - 400 + 10, from.y() + 50); // bord gauche à 10 px
        QTest::mousePress(s.window, Qt::LeftButton, {}, from);
        QTest::mouseMove(s.window, from + QPoint(-10, 10));
        QTest::mouseMove(s.window, to);
        QTest::mouseRelease(s.window, Qt::LeftButton, {}, to);
        QTRY_COMPARE(profile->paletteDock(), QStringLiteral("left"));
    }

    void floatingUsesTheProfilePosition()
    {
        Shell s;
        QVERIFY(start(s));
        auto* profile = profileOf(s);
        profile->setPaletteDock(QStringLiteral("floating"));
        profile->setPaletteFloat(QPointF(300, 120));
        auto* palette = item(s, "toolPalette");
        QTRY_COMPARE(sceneOf(item(s, "palettePanel"), QPointF(0, 0)), QPointF(300, 120));
    }

    void collapsedShowsFirstRowOnly()
    {
        Shell s;
        QVERIFY(start(s));
        auto* rows = item(s, "paletteRows");
        QVERIFY(rows);
        QVERIFY(rows->property("shownRows").toInt() > 1);
        profileOf(s)->setPaletteCollapsed(true);
        QTRY_COMPARE(rows->property("shownRows").toInt(), 1);
        profileOf(s)->setPaletteCollapsed(false);
        QTRY_VERIFY(rows->property("shownRows").toInt() > 1);
    }

    void fullCollapseHidesShortcuts()
    {
        Shell s;
        QVERIFY(start(s));
        auto* bar = item(s, "shortcutBar");
        QVERIFY(bar);
        profileOf(s)->setShortcutsHidden(true);
        QTRY_VERIFY(!bar->isVisible());
    }

    void unpinnedHidesAfterLeave()
    {
        Shell s;
        QVERIFY(start(s));
        auto* profile = profileOf(s);
        profile->setPaletteDock(QStringLiteral("right"));
        profile->setPalettePinned(false);
        QTest::mouseMove(s.window, QPoint(10, s.window->height() / 2));
        auto* frame = item(s, "paletteFrame");
        QVERIFY(frame);
        QTRY_VERIFY_WITH_TIMEOUT(frame->property("folded").toBool(), 3000);
        QTest::mouseMove(s.window, QPoint(s.window->width() - 2, s.window->height() / 2));
        QTRY_VERIFY_WITH_TIMEOUT(!frame->property("folded").toBool(), 3000);
    }

    void pinnedNeverFolds()
    {
        Shell s;
        QVERIFY(start(s));
        profileOf(s)->setPaletteDock(QStringLiteral("right"));
        profileOf(s)->setPalettePinned(true);
        QTest::mouseMove(s.window, QPoint(10, s.window->height() / 2));
        QTest::qWait(1000);
        QVERIFY(!item(s, "paletteFrame")->property("folded").toBool());
    }

    void iconPixelsScaleEveryIcon()
    {
        Shell s;
        QVERIFY(start(s));
        profileOf(s)->setIconPixels(32);
        QList<QQuickItem*> buttons;
        collectButtons(s.window->contentItem(), buttons);
        QVERIFY(!buttons.isEmpty());
        for (QQuickItem* button : buttons) {
            QQmlProperty icon(button, QStringLiteral("icon.width"));
            if (!icon.isValid()) {
                continue;
            }
            QTRY_COMPARE_WITH_TIMEOUT(icon.read().toInt(), 32, 3000);
        }
    }

    void commandsDriveTheProfile()
    {
        Shell s;
        QVERIFY(start(s));
        auto* profile = profileOf(s);
        auto* commands = commandsOf(s);
        QVERIFY(profile && commands);
        QVERIFY(commands->trigger(QStringLiteral("palette.dock-top")));
        QCOMPARE(profile->paletteDock(), QStringLiteral("top"));
        QVERIFY(commands->trigger(QStringLiteral("palette.dock-bottom")));
        QCOMPARE(profile->paletteDock(), QStringLiteral("bottom"));
        QVERIFY(commands->trigger(QStringLiteral("palette.dock-left")));
        QCOMPARE(profile->paletteDock(), QStringLiteral("left"));
        QVERIFY(commands->trigger(QStringLiteral("palette.dock-right")));
        QCOMPARE(profile->paletteDock(), QStringLiteral("right"));
        QVERIFY(commands->trigger(QStringLiteral("palette.float")));
        QCOMPARE(profile->paletteDock(), QStringLiteral("floating"));
        QVERIFY(commands->trigger(QStringLiteral("palette.collapse")));
        QVERIFY(profile->paletteCollapsed());
        QVERIFY(commands->isChecked(QStringLiteral("palette.collapse")));
        QVERIFY(commands->trigger(QStringLiteral("palette.collapse")));
        QVERIFY(!profile->paletteCollapsed());
        QVERIFY(!commands->isChecked(QStringLiteral("palette.collapse")));
        QVERIFY(commands->trigger(QStringLiteral("palette.collapse-full")));
        QVERIFY(profile->shortcutsHidden());
        QVERIFY(commands->isChecked(QStringLiteral("palette.collapse-full")));
        QVERIFY(commands->trigger(QStringLiteral("palette.collapse-full")));
        QVERIFY(!profile->shortcutsHidden());
        const bool pinned = profile->palettePinned();
        QVERIFY(commands->trigger(QStringLiteral("palette.pin")));
        QCOMPARE(profile->palettePinned(), !pinned);
        QVERIFY(commands->trigger(QStringLiteral("palette.icon-size"), 20));
        QCOMPARE(profile->iconPixels(), 20);
        QVERIFY(commands->trigger(QStringLiteral("palette.icon-size"), 99));
        QCOMPARE(profile->iconPixels(), 32); // borné à 32 par le profil
    }
};

QTEST_MAIN(TstUiPalette)
#include "tst_ui_palette.moc"
