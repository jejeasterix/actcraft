#include <actcraft/ui/commands.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/profilecontroller.h>

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
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
#include <stdexcept>

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
        QSignalSpy layout(&commands, &ui::Commands::layoutChanged);
        QVERIFY(commands.trigger(QStringLiteral("profile.activate"), QStringLiteral("en-classe")));
        QCOMPARE(c->currentId(), QStringLiteral("en-classe"));
        // Rechargement différé pendant le gestionnaire, joué une fois au retour de trigger().
        QCOMPARE(layout.count(), 1);
        commands.reload();
        QVERIFY(commands.trigger(QStringLiteral("profile.activate"), QStringLiteral("calcul-geometrie")));
        QCOMPARE(c->currentId(), QStringLiteral("calcul-geometrie"));
    }

    // reload() reconstruit le registre : l'état actif posé par l'interface (Annuler sans historique) survit à
    // une activation, par la commande (rechargement différé) comme par l'appel direct.
    void activateKeepsCommandState()
    {
        ui::Commands commands;
        ui::Preferences prefs(ini(), nullptr);
        auto c = controller(&prefs, &commands);
        const QString undo = QStringLiteral("edit.undo");
        commands.setEnabled(undo, false); // document sans historique (Main.qml, syncEditCommands)
        QVERIFY(commands.trigger(QStringLiteral("profile.activate"), QStringLiteral("en-classe")));
        QCOMPARE(c->currentId(), QStringLiteral("en-classe"));
        QVERIFY(!commands.isEnabled(undo));
        QVERIFY(c->activate(QStringLiteral("calcul-geometrie")));
        QVERIFY(!commands.isEnabled(undo));
    }

    // Verrou de classe (L6-T3) gardé par reload(), et l'état propre recopié, pas l'état vu à travers le
    // verrou : recopier isEnabled() éteindrait pour de bon une commande verrouillable rechargée sous verrou.
    void reloadKeepsTheLock()
    {
        ui::Commands commands;
        const QString lockable = QStringLiteral("file.new");
        const QString undo = QStringLiteral("edit.undo");
        commands.setEnabled(undo, false);
        commands.setLocked(true);
        QVERIFY(!commands.isEnabled(lockable));
        commands.reload();
        QVERIFY(commands.isLocked());
        QVERIFY(!commands.isEnabled(lockable));
        commands.setLocked(false);
        QVERIFY(commands.isEnabled(lockable));
        QVERIFY(!commands.isEnabled(undo));
    }

    // Un gestionnaire qui lève une exception rend la profondeur : reload() n'est plus différé à jamais.
    void throwingHandlerReleasesTheDepth()
    {
        ui::Commands commands;
        commands.setNativeHandler(QStringLiteral("profile.activate"),
                                  [](const QVariant&) { throw std::runtime_error("essai"); });
        bool thrown = false;
        try {
            commands.trigger(QStringLiteral("profile.activate"), QStringLiteral("en-classe"));
        } catch (const std::runtime_error&) {
            thrown = true;
        }
        QVERIFY(thrown);
        QSignalSpy layout(&commands, &ui::Commands::layoutChanged);
        commands.reload();
        QCOMPARE(layout.count(), 1); // rechargé tout de suite
    }

    // Écriture refusée : la modification reste en attente et part à l'enregistrement suivant.
    void failedSaveIsRetried()
    {
        QFile blocker(profileRoot()); // un fichier à la place du dossier des profils : écriture impossible
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        blocker.close();
        ui::Commands commands;
        ui::Preferences prefs(ini(), nullptr);
        auto c = controller(&prefs, &commands);
        const int before = c->saveCount();
        QSignalSpy failed(c.get(), &ui::ProfileController::failed);
        const QString dock =
            c->paletteDock() == QStringLiteral("left") ? QStringLiteral("right") : QStringLiteral("left");
        c->setPaletteDock(dock);
        c->saveNow();
        QCOMPARE(failed.count(), 1);
        QCOMPARE(c->saveCount(), before);
        QVERIFY(QFile::remove(profileRoot()));
        c->saveNow();
        QCOMPARE(c->saveCount(), before + 1);
        QVERIFY(c->store().hasUserCopy(c->currentId()));
    }

    // Activer pendant un contact n'écrit rien : le profil quitté part au relâché, et le reprendre avant
    // rend sa modification en attente (pas la version du disque).
    void activateDuringContactWritesAtRelease()
    {
        ui::Commands commands;
        ui::Preferences prefs(ini(), nullptr);
        auto c = controller(&prefs, &commands);
        const QString left = c->currentId();
        const QString other = QStringLiteral("en-classe");
        QVERIFY(left != other);
        const int before = c->saveCount();
        const QString dock =
            c->paletteDock() == QStringLiteral("left") ? QStringLiteral("right") : QStringLiteral("left");
        c->setContactActive(true);
        c->setPaletteDock(dock);
        QVERIFY(c->activate(other));
        QCOMPARE(c->saveCount(), before);
        QVERIFY(!c->store().hasUserCopy(left));
        QVERIFY(c->activate(left));
        QCOMPARE(c->paletteDock(), dock);
        QVERIFY(c->activate(other));
        QTest::qWait(ui::ProfileController::kSaveDelayMs * 2);
        QCOMPARE(c->saveCount(), before);
        c->setContactActive(false);
        QTRY_COMPARE_WITH_TIMEOUT(c->saveCount(), before + 1, 2000);
        QVERIFY(c->store().hasUserCopy(left));
        QVERIFY(c->activate(left));
        QCOMPARE(c->paletteDock(), dock);
    }

    // Garde-fou (N1) : tout test qui charge la fenêtre principale isole les réglages (INI dans un dossier
    // jetable) et les profils (profileRoot). Main.qml crée Preferences {} (QSettings()) et active un profil
    // au démarrage : sans isolation, chaque passage écrit le profil courant dans les réglages du poste.
    void mainWindowTestsAreIsolated()
    {
        const QDir tests = QFileInfo(QString::fromUtf8(__FILE__)).absoluteDir();
        QVERIFY2(tests.exists(QStringLiteral("tst_ui_preferences.cpp")), qPrintable(tests.path()));
        // Relevés hors du périmètre de L6-T5 (2026-11-04) : à isoler, puis à retirer de cette liste.
        const QStringList pending{QStringLiteral("tst_ui_lot5tools.cpp"), QStringLiteral("tst_ui_shapes.cpp"),
                                  QStringLiteral("tst_ui_tools.cpp")};
        const QRegularExpression loadsMain(
            QStringLiteral(R"(loadFromModule\("ActCraft\.Ui",\s*"Main"\)|loadMainWindow\()"));
        int loaders = 0;
        QStringList missing;
        const QFileInfoList files = tests.entryInfoList({QStringLiteral("*.cpp")}, QDir::Files);
        for (const QFileInfo& file : files) {
            QFile f(file.absoluteFilePath());
            QVERIFY2(f.open(QIODevice::ReadOnly), qPrintable(file.fileName()));
            const QString source = QString::fromUtf8(f.readAll());
            if (!loadsMain.match(source).hasMatch() || pending.contains(file.fileName())) {
                continue;
            }
            ++loaders;
            for (const char* needed : {"QSettings::setDefaultFormat(QSettings::IniFormat)",
                                       "QSettings::setPath(QSettings::IniFormat", "profileRoot"}) {
                if (!source.contains(QLatin1String(needed))) {
                    missing << file.fileName() + QStringLiteral(" : ") + QLatin1String(needed);
                }
            }
        }
        QVERIFY(loaders >= 5);
        QVERIFY2(missing.isEmpty(), qPrintable(missing.join(QLatin1Char('\n'))));
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
