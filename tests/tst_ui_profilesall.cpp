// L11-T27 (S32) : les cinq profils fournis de bout en bout, sur la vraie fenêtre (Main.qml).
#include "startup.h"

#include <actcraft/actions/commandregistry.h>
#include <actcraft/actions/profile.h>
#include <actcraft/actions/profilestore.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/profilecontroller.h>

#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

using namespace actcraft;

class TstUiProfilesall : public QObject
{
    Q_OBJECT
    QTemporaryDir m_settings;
    QTemporaryDir m_profiles;
    std::unique_ptr<QQmlApplicationEngine> m_engine;
    QQuickWindow* m_window = nullptr;
    ui::Commands* m_commands = nullptr;
    ui::ProfileController* m_profile = nullptr;
    ui::Preferences* m_preferences = nullptr;

    QStringList ids() const { return ui::ProfileController::builtinOrder(); }
    const actions::CommandRegistry& registry() const { return m_commands->registry(); }
    bool activate(const QString& id)
    {
        return m_commands->trigger(QStringLiteral("profile.activate"), id) && m_profile->currentId() == id;
    }
    // Groupe « profil » de la palette principale (commandes épinglées), tel que le QML le calcule.
    QStringList paletteProfileGroup() const
    {
        auto* palette = m_window->findChild<QQuickItem*>(QStringLiteral("toolPalette"));
        if (!palette)
            return {QStringLiteral("<palette absente>")};
        for (const QVariant& group : palette->property("groups").toList()) {
            const QVariantMap map = group.toMap();
            if (map.value(QStringLiteral("group")).toString() == QStringLiteral("profil"))
                return map.value(QStringLiteral("items")).toStringList();
        }
        return {};
    }

private slots:
    // Réglages d'essai : aucun réglage ni dossier du poste touché.
    void initTestCase()
    {
        QVERIFY(m_settings.isValid());
        QVERIFY(m_profiles.isValid());
        QStandardPaths::setTestModeEnabled(true);
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
        QSettings probe;
        QCOMPARE(probe.format(), QSettings::IniFormat);
        QVERIFY(probe.fileName().startsWith(m_settings.path()));

        StartupOptions options;
        options.profileRoot = m_profiles.path(); // profil vierge : aucun fichier d'utilisateur
        m_engine = std::make_unique<QQmlApplicationEngine>();
        m_window = loadMainWindow(*m_engine, options);
        QVERIFY(m_window);
        m_commands = m_window->findChild<ui::Commands*>(QStringLiteral("commands"));
        m_profile = m_window->findChild<ui::ProfileController*>(QStringLiteral("appProfile"));
        m_preferences = m_window->findChild<ui::Preferences*>(QStringLiteral("appPreferences"));
        QVERIFY(m_commands);
        QVERIFY(m_profile);
        QVERIFY(m_preferences);
    }
    void cleanupTestCase() { m_engine.reset(); }

    void everyPinnedCommandExists()
    {
        for (const QString& id : ids()) {
            QVERIFY2(activate(id), qPrintable(id));
            for (const QString& command : m_profile->pinned())
                QVERIFY2(registry().find(command), qPrintable(id + QStringLiteral(" : ") + command));
            // Information pour le rapport : épinglages sans gestionnaire (lot 7 pas encore fusionné).
            QStringList inert;
            for (const QString& command : m_profile->pinned())
                if (!registry().hasHandler(command))
                    inert << command;
            if (!inert.isEmpty())
                qInfo().noquote() << "SANS GESTIONNAIRE" << id << inert.join(QLatin1Char(' '));
        }
    }

    void activatedFromBlankShowsEveryPin()
    {
        const actions::Platform platform = actions::currentPlatform();
        for (const QString& id : ids()) {
            QVERIFY2(activate(id), qPrintable(id));
            QCOMPARE(m_preferences->currentProfile(), id);
            const QStringList expected =
                actions::layoutForPlatform(m_profile->profile(), registry(), platform);
            for (const QString& command : m_profile->pinned()) {
                if (!registry().isAvailable(command, platform))
                    continue;
                QVERIFY2(expected.contains(command), qPrintable(id + QStringLiteral(" : ") + command));
                QVERIFY2(m_commands->isAvailable(command), qPrintable(id + QStringLiteral(" : ") + command));
                // Barre de raccourcis : une commande épinglée qui a une séquence y figure.
                if (!m_commands->shortcut(command).isEmpty()) {
                    bool found = false;
                    for (const QVariant& entry : m_commands->shortcuts())
                        found = found || entry.toMap().value(QStringLiteral("id")).toString() == command;
                    QVERIFY2(found, qPrintable(id + QStringLiteral(" : raccourci de ") + command));
                }
            }
            // Palette : le groupe « profil » du QML est exactement la disposition effective.
            QCOMPARE(paletteProfileGroup(), expected);
        }
    }

    void browsersAndSettingsApplied()
    {
        QString error;
        auto store = actions::ProfileStore(QString(), QStringLiteral(":/actcraft/actions/profiles"),
                                           ui::ProfileController::builtinOrder());
        for (const QString& id : ids()) {
            const auto p = store.load(id, error);
            QVERIFY2(p, qPrintable(error));
            QVERIFY2(activate(id), qPrintable(id));
            QCOMPARE(m_profile->browsersVisible(), p->browsers.visible);
            QCOMPARE(m_profile->browsersSide(), p->browsers.side);
            QCOMPARE(m_profile->browsersPinned(), p->browsers.pinned);
            QCOMPARE(m_profile->browsersCurrent(), p->browsers.current);
            QCOMPARE(m_profile->swatches(), p->palette.swatches);
            QCOMPARE(m_profile->iconPixels(), p->iconPixels);
            QCOMPARE(m_profile->paletteDock(), actions::dockName(p->palette.dock));
            QCOMPARE(m_profile->fitPage(), p->fitPage);
            QCOMPARE(m_profile->trashVisible(), p->trashVisible);
        }
        // Les réglages propres au profil de préparation (F-930) sont relus.
        QVERIFY(activate(QStringLiteral("preparation")));
        QVERIFY(m_profile->browsersVisible());
        QCOMPARE(m_profile->swatches(), 24);
        QCOMPARE(m_profile->setting(QStringLiteral("defaults"), QStringLiteral("page-size"), QString()).toString(),
                 QStringLiteral("1024x768"));
        QVERIFY(activate(QStringLiteral("en-classe")));
        QVERIFY(!m_profile->browsersVisible());
        QVERIFY(m_profile->fitPage());
    }

    void noAndroidAbsentCommandOnAndroid()
    {
        QString error;
        auto store = actions::ProfileStore(QString(), QStringLiteral(":/actcraft/actions/profiles"),
                                           ui::ProfileController::builtinOrder());
        int dropped = 0;
        for (const QString& id : ids()) {
            const auto p = store.load(id, error);
            QVERIFY2(p, qPrintable(error));
            const QStringList android =
                actions::layoutForPlatform(*p, registry(), actions::Platform::Android);
            for (const QString& command : android) {
                const auto* info = registry().find(command);
                QVERIFY2(info, qPrintable(command));
                QVERIFY2((info->platforms & static_cast<actions::Platforms>(actions::Platform::Android)) != 0,
                         qPrintable(id + QStringLiteral(" : ") + command));
            }
            for (const QString& command : p->pinned)
                if (!android.contains(command)) {
                    ++dropped;
                    QVERIFY2(!registry().isAvailable(command, actions::Platform::Android),
                             qPrintable(command));
                }
            // Sous Windows, tout ce qui est déclaré pour Windows reste.
            for (const QString& command : actions::layoutForPlatform(*p, registry(), actions::Platform::Windows))
                QVERIFY2(registry().isAvailable(command, actions::Platform::Windows), qPrintable(command));
        }
        QVERIFY(dropped >= 1);
        const auto son = store.load(QStringLiteral("son-image"), error);
        QVERIFY(son);
        const QStringList android = actions::layoutForPlatform(*son, registry(), actions::Platform::Android);
        QVERIFY(!android.contains(QStringLiteral("record.screen-full")));
        QVERIFY(!android.contains(QStringLiteral("record.screen-area")));
        QVERIFY(android.contains(QStringLiteral("record.sound")));
    }

    // Épinglages du lot 8 (liste recopiée du rapport du lot 8). Ceux du lot 7 ne sont pas vérifiés ici : le lot 7
    // n'est pas fusionné à la tête du lot 11 (à raccorder à sa fusion).
    void lot7And8PinsPresent()
    {
        const QStringList lot8{QStringLiteral("media.sound-controller"), QStringLiteral("record.sound"),
                               QStringLiteral("record.screen-full"),     QStringLiteral("record.screen-area"),
                               QStringLiteral("insert.web-frame"),       QStringLiteral("link.file"),
                               QStringLiteral("link.website")};
        QVERIFY(activate(QStringLiteral("son-image")));
        const QStringList windows =
            actions::layoutForPlatform(m_profile->profile(), registry(), actions::Platform::Windows);
        const QStringList android =
            actions::layoutForPlatform(m_profile->profile(), registry(), actions::Platform::Android);
        for (const QString& command : lot8) {
            QVERIFY2(m_profile->pinned().contains(command), qPrintable(command));
            QVERIFY2(windows.contains(command), qPrintable(command));
            QCOMPARE(android.contains(command), !command.startsWith(QStringLiteral("record.screen-")));
        }
        QCOMPARE(m_profile->pinned().count(QStringLiteral("insert.image")), 1);
    }
};

QTEST_MAIN(TstUiProfilesall)
#include "tst_ui_profilesall.moc"
