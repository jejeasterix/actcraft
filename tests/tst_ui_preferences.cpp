#include <actcraft/ui/commands.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/profilecontroller.h>

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

// Réglages du poste et profil courant (L6-T5). Chaque fonction a son dossier : réglages INI et profils
// d'essai, jamais ceux du poste (amendement 3, N1).
class TstUiPreferences : public QObject
{
    Q_OBJECT
    QTemporaryDir m_settings; // réglages par défaut (QSettings()) : format INI posé dans initTestCase
    std::unique_ptr<QTemporaryDir> m_dir;

    QString ini() const { return m_dir->filePath(QStringLiteral("poste.ini")); }
    QString profileRoot() const { return m_dir->filePath(QStringLiteral("profiles")); }

    std::unique_ptr<ui::ProfileController> controller(ui::Preferences* prefs, ui::Commands* commands,
                                                      bool started = true)
    {
        actions::ProfileStore store(profileRoot(), QStringLiteral(":/actcraft/actions/profiles"),
                                    ui::ProfileController::builtinOrder());
        auto c = std::make_unique<ui::ProfileController>(std::move(store), prefs, commands);
        if (started) {
            c->start();
        }
        return c;
    }

    // Commands, Preferences et ProfileController instanciés comme dans Main.qml.
    std::unique_ptr<QObject> fromQml(QQmlEngine& engine)
    {
        QQmlComponent component(&engine);
        component.setData(QStringLiteral("import QtQuick\n"
                                         "import ActCraft.Ui\n"
                                         "Item {\n"
                                         "    Commands { id: c; objectName: \"commands\" }\n"
                                         "    Preferences { id: p; objectName: \"appPreferences\" }\n"
                                         "    ProfileController {\n"
                                         "        objectName: \"appProfile\"\n"
                                         "        commands: c\n"
                                         "        preferences: p\n"
                                         "        userRoot: \"%1\"\n"
                                         "    }\n"
                                         "}\n")
                              .arg(profileRoot())
                              .toUtf8(),
                          QUrl());
        std::unique_ptr<QObject> root(component.create());
        if (!root) {
            qWarning("%s", qPrintable(component.errorString()));
        }
        return root;
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_settings.isValid());
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setOrganizationName(QStringLiteral("ActCraft"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
    }

    void init()
    {
        m_dir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_dir->isValid());
        QSettings().clear();
    }

    void keysAreOurs()
    {
        ui::Preferences prefs(ini(), nullptr);
        prefs.setCurrentProfile(QStringLiteral("en-classe"));
        prefs.setDashboardAtStart(false);
        prefs.setLanguage(QStringLiteral("fr"));
        prefs.setValue(QString::fromLatin1(ui::Preferences::kRecent), QStringList{QStringLiteral("a.acb")});
        const QRegularExpression kebab(QStringLiteral("^[a-z]+(-[a-z]+)*(/[a-z]+(-[a-z]+)*)+$"));
        QCOMPARE(prefs.allKeys().size(), 4);
        for (const QString& key : prefs.allKeys()) {
            QVERIFY2(kebab.match(key).hasMatch(), qPrintable(key));
        }
    }

    void currentProfileRestored()
    {
        ui::Commands commands;
        {
            ui::Preferences prefs(ini(), nullptr);
            auto c = controller(&prefs, &commands);
            QVERIFY(c->activate(QStringLiteral("calcul-geometrie")));
        }
        ui::Preferences prefs(ini(), nullptr);
        QCOMPARE(prefs.currentProfile(), QStringLiteral("calcul-geometrie"));
        auto c = controller(&prefs, &commands);
        QCOMPARE(c->currentId(), QStringLiteral("calcul-geometrie"));
    }

    // Le patron écarté (enregistrer dans chaque setter) donnerait 17 enregistrements tout de suite.
    void implicitSaveIsDebounced()
    {
        ui::Commands commands;
        ui::Preferences prefs(ini(), nullptr);
        auto c = controller(&prefs, &commands);
        const int before = c->saveCount();
        QSignalSpy changed(c.get(), &ui::ProfileController::profileChanged);
        for (int i = 16; i <= 32; ++i) {
            c->setIconPixels(i);
        }
        QCOMPARE(c->iconPixels(), 32);
        QVERIFY(changed.count() >= 1);
        QCOMPARE(c->saveCount(), before);
        QTRY_COMPARE_WITH_TIMEOUT(c->saveCount(), before + 1, 2000);
        QTest::qWait(ui::ProfileController::kSaveDelayMs + 200);
        QCOMPARE(c->saveCount(), before + 1);
        // L'unique enregistrement porte la dernière valeur.
        QString error;
        const auto saved = c->store().load(c->currentId(), error);
        QVERIFY2(saved.has_value(), qPrintable(error));
        QCOMPARE(saved->iconPixels, 32);
    }

    void noSaveDuringContact()
    {
        ui::Commands commands;
        ui::Preferences prefs(ini(), nullptr);
        auto c = controller(&prefs, &commands);
        const int before = c->saveCount();
        QSignalSpy contact(c.get(), &ui::ProfileController::contactActiveChanged);
        c->setContactActive(true);
        QCOMPARE(contact.count(), 1);
        c->setPaletteDock(QStringLiteral("left"));
        QCOMPARE(c->paletteDock(), QStringLiteral("left"));
        QTest::qWait(ui::ProfileController::kSaveDelayMs * 3);
        QCOMPARE(c->saveCount(), before);
        QVERIFY(!c->store().hasUserCopy(c->currentId()));
        c->setContactActive(false);
        QCOMPARE(c->saveCount(), before); // différé encore après le relâché
        QTRY_COMPARE_WITH_TIMEOUT(c->saveCount(), before + 1, 2000);
        QVERIFY(c->store().hasUserCopy(c->currentId()));
        // Un contact sans modification n'enregistre rien.
        c->setContactActive(true);
        c->setContactActive(false);
        QTest::qWait(ui::ProfileController::kSaveDelayMs * 2);
        QCOMPARE(c->saveCount(), before + 1);
    }

    void activateSwitchesTheLayoutDir()
    {
        ui::Commands commands;
        ui::Preferences prefs(ini(), nullptr);
        auto c = controller(&prefs, &commands);
        QVERIFY(QDir().mkpath(c->store().directory(QStringLiteral("en-classe"))));
        QFile f(c->store().directory(QStringLiteral("en-classe")) + QStringLiteral("/layout.json"));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("{"); // disposition illisible : erreur visible dans Commands::errors
        f.close();
        QVERIFY(c->activate(QStringLiteral("en-classe")));
        QVERIFY(!commands.errors().isEmpty());
        QVERIFY(c->activate(QStringLiteral("preparation")));
        QVERIFY(commands.errors().isEmpty());
    }

    void brokenProfileFallsBack()
    {
        ui::Commands commands;
        ui::Preferences prefs(ini(), nullptr);
        prefs.setCurrentProfile(QStringLiteral("absent"));
        auto c = controller(&prefs, &commands);
        QCOMPARE(c->currentId(), ui::ProfileController::defaultProfileId());
        prefs.setCurrentProfile(QStringLiteral("../preparation"));
        auto d = controller(&prefs, &commands);
        QCOMPARE(d->currentId(), ui::ProfileController::defaultProfileId());
    }

    void settingDefaultsComeFromTheSchema()
    {
        ui::Commands commands;
        ui::Preferences prefs(ini(), nullptr);
        auto c = controller(&prefs, &commands);
        QCOMPARE(c->setting(QStringLiteral("objects"), QStringLiteral("auto-group"), false).toBool(), false);
        c->setSetting(QStringLiteral("objects"), QStringLiteral("auto-group"), true);
        QCOMPARE(c->setting(QStringLiteral("objects"), QStringLiteral("auto-group"), false).toBool(), true);
    }

    // Gestionnaire C++ gardé à travers reload() : activer recharge la disposition (setProfileDir).
    void activateCommandIsNative()
    {
        ui::Commands commands;
        ui::Preferences prefs(ini(), nullptr);
        auto c = controller(&prefs, &commands);
        QVERIFY(commands.trigger(QStringLiteral("profile.activate"), QStringLiteral("en-classe")));
        QCOMPARE(c->currentId(), QStringLiteral("en-classe"));
        commands.reload();
        QVERIFY(commands.trigger(QStringLiteral("profile.activate"), QStringLiteral("calcul-geometrie")));
        QCOMPARE(c->currentId(), QStringLiteral("calcul-geometrie"));
    }

    void pathLikeIdRefused()
    {
        ui::Commands commands;
        ui::Preferences prefs(ini(), nullptr);
        auto c = controller(&prefs, &commands);
        const QString before = c->currentId();
        QSignalSpy failed(c.get(), &ui::ProfileController::failed);
        const QStringList ids{QStringLiteral("../preparation"), QStringLiteral("a/b"),
                              QStringLiteral("En-Classe"), QString()};
        for (const QString& id : ids) {
            QVERIFY2(!c->activate(id), qPrintable(id));
        }
        QCOMPARE(failed.count(), ids.size());
        QCOMPARE(c->currentId(), before);
        QCOMPARE(prefs.currentProfile(), before);
    }

    // Le joueur et le banc posent le format INI par défaut : Preferences() le suit (jamais NativeFormat).
    void defaultConstructorFollowsTheDefaultFormat()
    {
        {
            ui::Preferences prefs;
            prefs.setLanguage(QStringLiteral("fr"));
        }
        QDirIterator it(m_settings.path(), {QStringLiteral("*.ini")}, QDir::Files,
                        QDirIterator::Subdirectories);
        QVERIFY(it.hasNext());
        QSettings settings;
        QCOMPARE(settings.format(), QSettings::IniFormat);
        QCOMPARE(settings.value(QString::fromLatin1(ui::Preferences::kLanguage)).toString(),
                 QStringLiteral("fr"));
    }

    void createdFromQml()
    {
        QQmlEngine engine;
        auto root = fromQml(engine);
        QVERIFY(root);
        QObject* profile = root->findChild<QObject*>(QStringLiteral("appProfile"));
        QObject* prefs = root->findChild<QObject*>(QStringLiteral("appPreferences"));
        QVERIFY(profile);
        QVERIFY(prefs);
        QCOMPARE(profile->property("currentId").toString(), QStringLiteral("preparation"));
        QCOMPARE(prefs->property("currentProfile").toString(), QStringLiteral("preparation"));
        // userRoot de l'instance : la disposition et les copies vont sous le dossier d'essai.
        auto* controller = qobject_cast<ui::ProfileController*>(profile);
        QVERIFY(controller);
        QCOMPARE(controller->store().directory(QStringLiteral("en-classe")),
                 profileRoot() + QStringLiteral("/en-classe"));
    }

    // start() : par componentComplete seul, jamais sur un changement de commands ou de preferences (n3).
    void startsOnceFromComponentComplete()
    {
        {
            ui::Commands commands;
            ui::Preferences prefs(ini(), nullptr);
            auto c = controller(&prefs, &commands, false);
            QCOMPARE(c->startCount(), 0);
            c->start();
            QCOMPARE(c->startCount(), 1);
        }
        QQmlEngine engine;
        auto root = fromQml(engine);
        QVERIFY(root);
        auto* controller = root->findChild<ui::ProfileController*>(QStringLiteral("appProfile"));
        QVERIFY(controller);
        QCOMPARE(controller->startCount(), 1);
        ui::Commands otherCommands;
        ui::Preferences otherPrefs(ini(), nullptr);
        controller->setPreferences(&otherPrefs);
        controller->setCommands(&otherCommands);
        controller->setPreferences(nullptr);
        QCOMPARE(controller->startCount(), 1);
        root.reset();
    }
};

QTEST_MAIN(TstUiPreferences)
#include "tst_ui_preferences.moc"
