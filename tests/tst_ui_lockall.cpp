#include <actcraft/actions/commandregistry.h>
#include <actcraft/ui/classroomlock.h>
#include <actcraft/ui/commands.h>

#include "startup.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QQmlApplicationEngine>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

using namespace actcraft;

namespace {

const QString kPassword = QStringLiteral("essai-1234"); // le seul mot de passe d'essai (celui du scénario)

// Nom de la plateforme tel que commands.json l'écrit, tenu ici à part du registre : le compte attendu
// ne doit rien devoir au code qu'il contrôle.
QString jsonPlatformName(actions::Platform platform)
{
    switch (platform) {
    case actions::Platform::Windows:
        return QStringLiteral("windows");
    case actions::Platform::Android:
        return QStringLiteral("android");
    case actions::Platform::MacOS:
        return QStringLiteral("macos");
    case actions::Platform::Linux:
        return QStringLiteral("linux");
    }
    return {};
}

QJsonArray rawCommands()
{
    return QJsonDocument::fromJson(actions::defaultCommandsJson())
        .object()
        .value(QStringLiteral("commands"))
        .toArray();
}

// Disponible sur `platform` d'après le JSON brut : `platforms` absent = partout.
bool rawAvailable(const QJsonObject& o, const QString& platform)
{
    const QJsonValue platforms = o.value(QStringLiteral("platforms"));
    return platforms.isUndefined() || platforms.toArray().contains(QJsonValue(platform));
}

// K : commandes verrouillables disponibles sur `platform`, comptées dans le JSON brut.
int rawLockedCount(const QString& platform)
{
    int count = 0;
    for (const QJsonValue& v : rawCommands()) {
        const QJsonObject o = v.toObject();
        if (o.value(QStringLiteral("lockable")).toBool() && rawAvailable(o, platform)) {
            ++count;
        }
    }
    return count;
}

} // namespace

// S19 rejoué sur toutes les commandes (L11-T26). Le verrou se lit sur l'instance `Commands` de Main.qml
// (objectName « commands », celle que lisent les scénarios et que ClassroomLock verrouille), jamais sur un
// registre chargé à part : un registre autonome passerait même si l'application comptait faux. Le compte
// attendu vient du JSON brut, jamais d'une boucle sur le registre.
class TstUiLockall : public QObject
{
    Q_OBJECT

    QTemporaryDir m_settings;
    QTemporaryDir m_profiles;
    QTemporaryDir m_recovery;
    std::unique_ptr<QQmlApplicationEngine> m_engine;
    ui::Commands* m_commands = nullptr;
    ui::ClassroomLock* m_lock = nullptr;

    bool lockApp()
    {
        if (!m_lock->property("hasPassword").toBool()) {
            if (!m_lock->createPassword(kPassword, kPassword)) {
                return false;
            }
            QTRY_VERIFY_WITH_TIMEOUT(m_lock->property("hasPassword").toBool(), 30000);
        }
        if (!m_commands->isLocked() && !m_lock->lock()) {
            return false;
        }
        return m_commands->isLocked();
    }

    bool unlockApp()
    {
        if (!m_commands->isLocked()) {
            return true;
        }
        if (!m_lock->unlock(kPassword)) {
            return false;
        }
        QTRY_VERIFY_WITH_TIMEOUT(!m_commands->isLocked(), 30000);
        return !m_commands->isLocked();
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_settings.isValid());
        QVERIFY(m_profiles.isValid());
        QVERIFY(m_recovery.isValid());
        QStandardPaths::setTestModeEnabled(true);
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());

        StartupOptions options;
        options.profileRoot = m_profiles.path();
        options.recoveryDir = m_recovery.path();
        m_engine = std::make_unique<QQmlApplicationEngine>();
        QVERIFY(loadMainQml(*m_engine, options));
        QVERIFY(!m_engine->rootObjects().isEmpty());
        QObject* root = m_engine->rootObjects().constFirst();
        m_commands = root->findChild<ui::Commands*>(QStringLiteral("commands"));
        QVERIFY2(m_commands, "objet « commands » absent de Main.qml");
        m_lock = root->findChild<ui::ClassroomLock*>();
        QVERIFY2(m_lock, "ClassroomLock absent de Main.qml");

        QSettings probe;
        QCOMPARE(probe.format(), QSettings::IniFormat);
        QVERIFY(probe.fileName().startsWith(m_settings.path()));
    }

    void cleanupTestCase()
    {
        if (m_commands && m_commands->isLocked()) {
            QVERIFY(unlockApp());
        }
        m_engine.reset();
    }

    // Lecture brute : chaque objet déclare lockable et restrictable, en booléens (aucun défaut tacite), et
    // le registre de l'application en a exactement autant.
    void everyCommandDeclaresBothAttributes()
    {
        const QJsonArray all = rawCommands();
        QVERIFY(!all.isEmpty());
        for (const QJsonValue& v : all) {
            const QJsonObject o = v.toObject();
            const QString id = o.value(QStringLiteral("id")).toString();
            QVERIFY2(o.value(QStringLiteral("lockable")).isBool(), qPrintable(id));
            QVERIFY2(o.value(QStringLiteral("restrictable")).isBool(), qPrintable(id));
            QVERIFY2(m_commands->registry().find(id), qPrintable(id));
        }
        QCOMPARE(m_commands->registry().ids().size(), all.size());
    }

    // Verrou posé par ClassroomLock : toute commande verrouillable disponible est grisée ; une commande
    // libre garde exactement son état propre. La forme « grisée ⇔ verrouillable » serait fausse ici : une
    // commande libre que son contrôleur désactive (page précédente en page 1) est grisée sans le verrou.
    void lockGreysExactlyTheLockables()
    {
        const QString platform = jsonPlatformName(actions::currentPlatform());
        QHash<QString, bool> ownBefore;
        for (const QJsonValue& v : rawCommands()) {
            const QString id = v.toObject().value(QStringLiteral("id")).toString();
            ownBefore.insert(id, m_commands->registry().isOwnEnabled(id));
        }
        QVERIFY(lockApp());
        int greyed = 0;
        for (const QJsonValue& v : rawCommands()) {
            const QJsonObject o = v.toObject();
            if (!rawAvailable(o, platform)) {
                continue;
            }
            const QString id = o.value(QStringLiteral("id")).toString();
            const bool own = m_commands->registry().isOwnEnabled(id);
            QVERIFY2(own == ownBefore.value(id), qPrintable(id)); // le verrou n'écrit pas l'état propre
            if (o.value(QStringLiteral("lockable")).toBool()) {
                QVERIFY2(!m_commands->isEnabled(id), qPrintable(id));
                ++greyed;
            } else {
                QVERIFY2(m_commands->isEnabled(id) == own, qPrintable(id));
            }
        }
        QCOMPARE(greyed, rawLockedCount(platform));
        QVERIFY(unlockApp());
    }

    // K = commandes verrouillables disponibles sur la plateforme, comptées dans le JSON brut ; lu par la
    // propriété que le joueur lit (« commands.lockedCount »), 0 hors du verrou.
    void lockedCountMatchesTheData()
    {
        const QString platform = jsonPlatformName(actions::currentPlatform());
        const int expected = rawLockedCount(platform);
        qInfo("K %s = %d", qPrintable(platform), expected);
        QCOMPARE(m_commands->property("lockedCount").toInt(), 0);
        QVERIFY(lockApp());
        QCOMPARE(m_commands->property("lockedCount").toInt(), expected);
        QCOMPARE(m_commands->lockedCount(), expected);
        QVERIFY(unlockApp());
        QCOMPARE(m_commands->property("lockedCount").toInt(), 0);
    }

    // Une commande verrouillable désactivée par son contrôleur avant le verrou reste désactivée après ;
    // toutes les autres retrouvent l'état d'avant.
    void unlockRestoresOwnState()
    {
        const QString platform = jsonPlatformName(actions::currentPlatform());
        QString chosen;
        for (const QJsonValue& v : rawCommands()) {
            const QJsonObject o = v.toObject();
            const QString id = o.value(QStringLiteral("id")).toString();
            if (o.value(QStringLiteral("lockable")).toBool() && rawAvailable(o, platform) &&
                m_commands->isEnabled(id)) {
                chosen = id;
                break;
            }
        }
        QVERIFY(!chosen.isEmpty());
        m_commands->setEnabled(chosen, false);

        QHash<QString, bool> before;
        for (const QString& id : m_commands->registry().ids()) {
            before.insert(id, m_commands->isEnabled(id));
        }
        QVERIFY(lockApp());
        QVERIFY(!m_commands->isEnabled(chosen));
        QVERIFY(unlockApp());
        QVERIFY2(!m_commands->isEnabled(chosen), qPrintable(chosen));
        QVERIFY2(!m_commands->registry().isOwnEnabled(chosen), qPrintable(chosen));
        for (const QString& id : m_commands->registry().ids()) {
            QVERIFY2(m_commands->isEnabled(id) == before.value(id), qPrintable(id));
        }
        m_commands->setEnabled(chosen, true);
    }

    // Registre filtré pour Android : le compte suit le JSON brut de cette plateforme, et le verrou survit
    // au rechargement qu'impose le changement de plateforme.
    void androidCountMatchesTheData()
    {
        const int expected = rawLockedCount(QStringLiteral("android"));
        qInfo("K android = %d", expected);
        QVERIFY(lockApp());
        m_commands->setPlatform(actions::Platform::Android);
        QVERIFY(m_commands->isLocked());
        QCOMPARE(m_commands->lockedCount(), expected);
        m_commands->setPlatform(actions::currentPlatform());
        QCOMPARE(m_commands->lockedCount(), rawLockedCount(jsonPlatformName(actions::currentPlatform())));
        QVERIFY(unlockApp());
    }
};

QTEST_MAIN(TstUiLockall)
#include "tst_ui_lockall.moc"
