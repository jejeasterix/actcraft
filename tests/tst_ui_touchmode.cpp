#include "startup.h"

#include <actcraft/ui/commands.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/touchmode.h>

#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

constexpr char kKey[] = "workstation/touch-mode";

// Commandes, réglages du poste (INI) et contrôleur ; open() rejoue une relance sur le même fichier.
struct Fixture
{
    QTemporaryDir dir;
    std::unique_ptr<ui::Commands> commands;
    std::unique_ptr<ui::Preferences> prefs;
    std::unique_ptr<ui::TouchModeController> controller;

    Fixture() { open(); }
    QString ini() const { return dir.filePath(QStringLiteral("poste.ini")); }
    void close()
    {
        controller.reset();
        prefs.reset(); // écrit le fichier
        commands.reset();
    }
    void open()
    {
        close();
        commands = std::make_unique<ui::Commands>();
        commands->setProfileDir(dir.filePath(QStringLiteral("disposition")));
        prefs = std::make_unique<ui::Preferences>(ini(), nullptr);
        controller = std::make_unique<ui::TouchModeController>();
        controller->setCommands(commands.get());
        controller->setPreferences(prefs.get());
    }
};

struct App
{
    QTemporaryDir profiles;
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::PageView* view = nullptr;
    ui::TouchModeController* controller = nullptr;
};

bool start(App& app)
{
    StartupOptions options;
    options.cacheEnabled = false;
    options.generatedPage = false;
    options.profileRoot = app.profiles.path();
    app.window = loadMainWindow(app.engine, options);
    if (!app.window || !QTest::qWaitForWindowExposed(app.window)) {
        return false;
    }
    app.view = app.window->findChild<ui::PageView*>(QStringLiteral("pageView"));
    app.controller = app.window->findChild<ui::TouchModeController*>(QStringLiteral("appTouchMode"));
    return app.view && app.controller;
}

} // namespace

class TstUiTouchmode : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_settings;
    QPointingDevice* m_touch = nullptr;

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY(m_settings.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
        m_touch = QTest::createTouchDevice();
        QSettings probe;
        QCOMPARE(probe.format(), QSettings::IniFormat);
        QVERIFY(probe.fileName().startsWith(m_settings.path()));
    }

    void init() { QSettings().remove(QString::fromLatin1(kKey)); }

    void fullByDefault()
    {
        Fixture f;
        QCOMPARE(f.controller->property("mode").toString(), QStringLiteral("full"));
    }

    void commandChangesModeAndSetting()
    {
        Fixture f;
        QVERIFY(f.commands->trigger(QStringLiteral("touch.mode"), QStringLiteral("pen")));
        QCOMPARE(f.controller->property("mode").toString(), QStringLiteral("pen"));
        QCOMPARE(f.prefs->value(QString::fromLatin1(kKey)).toString(), QStringLiteral("pen"));
        f.close();
        QSettings ini(f.ini(), QSettings::IniFormat);
        QCOMPARE(ini.value(QString::fromLatin1(kKey)).toString(), QStringLiteral("pen"));
    }

    void settingReadAtStart()
    {
        Fixture f;
        QVERIFY(f.commands->trigger(QStringLiteral("touch.mode"), QStringLiteral("select")));
        f.open();
        QCOMPARE(f.controller->property("mode").toString(), QStringLiteral("select"));
    }

    // Identifiant inconnu : le mode ne change pas et rien n'est écrit. Le registre ne connaît que la forme de
    // l'argument (texte non vide) : la commande est jouée, c'est le contrôleur qui refuse.
    void unknownRefused()
    {
        Fixture f;
        QVERIFY(f.commands->trigger(QStringLiteral("touch.mode"), QStringLiteral("ignore")));
        bool accepted = true;
        QVERIFY(QMetaObject::invokeMethod(f.controller.get(), "setMode", Q_RETURN_ARG(bool, accepted),
                                          Q_ARG(QString, QStringLiteral("zzz"))));
        QVERIFY(!accepted);
        f.commands->trigger(QStringLiteral("touch.mode"), QStringLiteral("zzz"));
        QVERIFY(!f.commands->trigger(QStringLiteral("touch.mode"), QStringLiteral("")));
        QCOMPARE(f.controller->property("mode").toString(), QStringLiteral("ignore"));
        QCOMPARE(f.prefs->value(QString::fromLatin1(kKey)).toString(), QStringLiteral("ignore"));
    }

    // Un changement demandé pendant un contact attend le relâché ; la vue n'est pas touchée avant.
    void changeDeferredDuringContact()
    {
        App app;
        QVERIFY(start(app));
        QCOMPARE(app.controller->property("mode").toString(), QStringLiteral("full"));
        const QPoint from = app.view->pageToScene(QPointF(300, 300)).toPoint();
        QTest::touchEvent(app.window, m_touch).press(0, from);
        QTest::touchEvent(app.window, m_touch).move(0, from + QPoint(20, 0));
        QTRY_VERIFY_WITH_TIMEOUT(app.view->contactActive(), 3000);
        QSignalSpy changed(app.controller, SIGNAL(modeChanged()));
        QVERIFY(app.controller->commands()->trigger(QStringLiteral("touch.mode"), QStringLiteral("ignore")));
        QCOMPARE(app.controller->property("mode").toString(), QStringLiteral("full"));
        QCOMPARE(changed.count(), 0);
        QTest::touchEvent(app.window, m_touch).release(0, from + QPoint(40, 0));
        QTRY_VERIFY_WITH_TIMEOUT(!app.view->contactActive(), 3000);
        QCOMPARE(app.controller->property("mode").toString(), QStringLiteral("ignore"));
        QCOMPARE(changed.count(), 1);
    }

    // La politique posée est celle du mode : un doigt posé est retenu en gestion complète, ignoré en « ignore ».
    void policyApplied()
    {
        App app;
        QVERIFY(start(app));
        const QPoint at = app.view->pageToScene(QPointF(300, 300)).toPoint();
        QVERIFY(app.controller->commands()->trigger(QStringLiteral("touch.mode"), QStringLiteral("ignore")));
        QCOMPARE(app.controller->property("mode").toString(), QStringLiteral("ignore"));
        QTest::touchEvent(app.window, m_touch).press(0, at);
        QTest::touchEvent(app.window, m_touch).move(0, at + QPoint(20, 0));
        QTest::qWait(100);
        QVERIFY(!app.view->contactActive());
        QTest::touchEvent(app.window, m_touch).release(0, at + QPoint(40, 0));
        QVERIFY(app.controller->commands()->trigger(QStringLiteral("touch.mode"), QStringLiteral("full")));
        QTest::touchEvent(app.window, m_touch).press(0, at);
        QTest::touchEvent(app.window, m_touch).move(0, at + QPoint(20, 0));
        QTRY_VERIFY_WITH_TIMEOUT(app.view->contactActive(), 3000);
        QTest::touchEvent(app.window, m_touch).release(0, at + QPoint(40, 0));
        QTRY_VERIFY_WITH_TIMEOUT(!app.view->contactActive(), 3000);
    }
};

QTEST_MAIN(TstUiTouchmode)
#include "tst_ui_touchmode.moc"
