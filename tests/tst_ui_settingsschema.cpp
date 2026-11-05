#include <actcraft/ui/designmode.h>
#include <actcraft/ui/networksettings.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/profilecontroller.h>
#include <actcraft/ui/settingsschema.h>

#include <QQmlComponent>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using actcraft::ui::DesignMode;
using actcraft::ui::NetworkSettings;
using actcraft::ui::Preferences;
using actcraft::ui::ProfileController;
using actcraft::ui::SettingsSchema;

namespace {

// Clés QSettings du lot 7 (A19), reprises telles quelles par le schéma.
const QStringList kLot7Keys{QStringLiteral("tools/ticker"), QStringLiteral("tools/calculator"),
                            QStringLiteral("tools/webHome"), QStringLiteral("presentation/transition")};

struct Fixture
{
    QTemporaryDir dir;
    Preferences preferences;
    ProfileController profile;
    SettingsSchema schema;

    Fixture()
    {
        profile.setPreferences(&preferences);
        profile.setUserRoot(dir.filePath(QStringLiteral("profiles")));
        profile.start();
        schema.setProfile(&profile);
        schema.setPreferences(&preferences);
    }
};

QStringList sectionIds(const SettingsSchema& schema)
{
    QStringList ids;
    for (const QVariant& section : schema.sections())
        ids << section.toMap().value(QStringLiteral("id")).toString();
    return ids;
}

QVariantMap entryOf(const SettingsSchema& schema, const QString& section, const QString& key)
{
    for (const QVariant& entry : schema.entries(section)) {
        const QVariantMap map = entry.toMap();
        if (map.value(QStringLiteral("key")).toString() == key)
            return map;
    }
    return {};
}

// Première section qui porte la clé, vide si aucune.
QString sectionOfKey(const SettingsSchema& schema, const QString& key)
{
    for (const QString& id : sectionIds(schema)) {
        if (!entryOf(schema, id, key).isEmpty())
            return id;
    }
    return {};
}

QSet<QString> settingsKeys()
{
    const QStringList keys = QSettings().allKeys();
    return QSet<QString>(keys.begin(), keys.end());
}

} // namespace

class TstUiSettingsschema : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setOrganizationName(QStringLiteral("ActCraftTests"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_ui_settingsschema"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QVERIFY(m_settingsDir.isValid());
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
    }

    void init() { QSettings().clear(); }

    void sectionsOfIncludedFunctionsOnly()
    {
        Fixture f;
        const QStringList expected{
            QStringLiteral("clock"),      QStringLiteral("effects"),  QStringLiteral("objects"),
            QStringLiteral("language"),   QStringLiteral("network"),  QStringLiteral("spelling"),
            QStringLiteral("tools"),      QStringLiteral("input"),    QStringLiteral("recordings"),
            QStringLiteral("profile"),    QStringLiteral("document"), QStringLiteral("multimedia"),
            QStringLiteral("design-mode")};
        QCOMPARE(sectionIds(f.schema), expected);
        for (const QVariant& value : f.schema.sections()) {
            const QVariantMap section = value.toMap();
            const QString id = section.value(QStringLiteral("id")).toString();
            QVERIFY2(!section.value(QStringLiteral("label")).toString().isEmpty(), qPrintable(id));
            const QStringList platforms = section.value(QStringLiteral("platforms")).toStringList();
            if (id == QLatin1String("network"))
                QCOMPARE(platforms, QStringList{QStringLiteral("windows")});
            else
                QVERIFY2(platforms.isEmpty(), qPrintable(id)); // toutes les plateformes
        }
    }

    void noVoteSection()
    {
        Fixture f;
        const QRegularExpression vote(QStringLiteral("vot"), QRegularExpression::CaseInsensitiveOption);
        QVERIFY(!f.schema.sections().isEmpty());
        for (const QVariant& value : f.schema.sections()) {
            const QVariantMap section = value.toMap();
            const QString id = section.value(QStringLiteral("id")).toString();
            QVERIFY2(!id.contains(vote), qPrintable(id));
            QVERIFY2(!section.value(QStringLiteral("label")).toString().contains(vote), qPrintable(id));
            for (const QVariant& entry : f.schema.entries(id)) {
                const QVariantMap map = entry.toMap();
                QVERIFY2(!map.value(QStringLiteral("key")).toString().contains(vote), qPrintable(id));
                QVERIFY2(!map.value(QStringLiteral("label")).toString().contains(vote), qPrintable(id));
            }
        }
    }

    void defaultsFromTheSpecification()
    {
        Fixture f;
        struct Expected
        {
            const char* section;
            const char* key;
            QVariant value;
        };
        const QList<Expected> expected{
            {"clock", "show-digits", true},        {"clock", "second-hand", true},
            {"clock", "twenty-four-hours", false}, {"effects", "spotlight-width", 15},
            {"effects", "curtain-speed", 15},      {"objects", "centering-tolerance", 10},
        };
        for (const Expected& e : expected) {
            const QString section = QString::fromLatin1(e.section);
            const QString key = QString::fromLatin1(e.key);
            const QVariantMap entry = entryOf(f.schema, section, key);
            QVERIFY2(!entry.isEmpty(), e.key);
            QCOMPARE(entry.value(QStringLiteral("defaultValue")), e.value);
            QCOMPARE(f.schema.value(section, key), e.value); // rien d'écrit : le défaut
        }
        // Les bornes encadrent le défaut des réglages numériques.
        for (const QString& id : sectionIds(f.schema)) {
            for (const QVariant& value : f.schema.entries(id)) {
                const QVariantMap entry = value.toMap();
                if (entry.value(QStringLiteral("type")).toString() != QLatin1String("int"))
                    continue;
                const int defaultValue = entry.value(QStringLiteral("defaultValue")).toInt();
                QVERIFY2(entry.value(QStringLiteral("min")).toInt() <= defaultValue,
                         qPrintable(entry.value(QStringLiteral("key")).toString()));
                QVERIFY2(defaultValue <= entry.value(QStringLiteral("max")).toInt(),
                         qPrintable(entry.value(QStringLiteral("key")).toString()));
            }
        }
    }

    // Le test qui distingue : la valeur passe par la clé du lot 7 elle-même, dans les deux sens, lue et
    // écrite en direct dans QSettings (jamais par un aller-retour du schéma, qui passerait avec une clé
    // dupliquée).
    void lot7KeysReused()
    {
        Fixture f;
        for (const QString& key : kLot7Keys) {
            const QString section = sectionOfKey(f.schema, key);
            QVERIFY2(!section.isEmpty(), qPrintable(key));
            const QVariantMap entry = entryOf(f.schema, section, key);
            QCOMPARE(entry.value(QStringLiteral("store")).toString(),
                     QString::fromLatin1(SettingsSchema::kWorkstationStore));
            QCOMPARE(entry.value(QStringLiteral("key")).toString(), key);
        }

        // Écrit par le lot 7 : lu par le schéma.
        const QString webHome = QStringLiteral("tools/webHome");
        QSettings().setValue(webHome, QStringLiteral("https://exemple.org/"));
        QCOMPARE(f.schema.value(sectionOfKey(f.schema, webHome), webHome).toString(),
                 QStringLiteral("https://exemple.org/"));

        // Écrit par le schéma : lu par le lot 7, sous sa clé, sans autre clé créée.
        const QString ticker = QStringLiteral("tools/ticker");
        const QSet<QString> before = settingsKeys();
        f.schema.setValue(sectionOfKey(f.schema, ticker), ticker, QStringLiteral("Bonjour"));
        QCOMPARE(QSettings().value(ticker).toString(), QStringLiteral("Bonjour"));
        QSet<QString> added = settingsKeys();
        added.subtract(before);
        QCOMPARE(added, QSet<QString>{ticker});
    }

    void profileOrWorkstationStore()
    {
        Fixture f;
        // Clés QSettings existantes : celles du lot 7, de Preferences et de NetworkSettings ; aucune neuve.
        QSet<QString> existing(kLot7Keys.begin(), kLot7Keys.end());
        existing << QString::fromLatin1(Preferences::kLanguage) << QString::fromLatin1(NetworkSettings::kHost)
                 << QString::fromLatin1(NetworkSettings::kPort)
                 << QString::fromLatin1(NetworkSettings::kUser);
        int profileEntries = 0;
        for (const QString& id : sectionIds(f.schema)) {
            for (const QVariant& value : f.schema.entries(id)) {
                const QVariantMap entry = value.toMap();
                const QString key = entry.value(QStringLiteral("key")).toString();
                const QString store = entry.value(QStringLiteral("store")).toString();
                if (store == QLatin1String(SettingsSchema::kWorkstationStore))
                    QVERIFY2(existing.contains(key), qPrintable(key));
                else if (store == QLatin1String(SettingsSchema::kProfileStore))
                    ++profileEntries;
                else
                    QFAIL(qPrintable(id + QLatin1Char('/') + key + QStringLiteral(" : magasin ") + store));
            }
        }
        QVERIFY(profileEntries > 0);

        // Un réglage du profil va dans le profil courant, et pas dans QSettings.
        const QSet<QString> before = settingsKeys();
        f.schema.setValue(QStringLiteral("objects"), QStringLiteral("centering-tolerance"), 25);
        QCOMPARE(
            f.profile.setting(QStringLiteral("objects"), QStringLiteral("centering-tolerance"), {}).toInt(),
            25);
        QCOMPARE(f.schema.value(QStringLiteral("objects"), QStringLiteral("centering-tolerance")).toInt(),
                 25);
        QSet<QString> added = settingsKeys();
        added.subtract(before);
        QVERIFY2(added.isEmpty(), qPrintable(QStringList(added.begin(), added.end()).join(QLatin1Char(','))));

        // Le mode conception (L6-T22) relit ce que le schéma écrit : même section du profil, même clé.
        f.schema.setValue(QStringLiteral("design-mode"), QStringLiteral("showHidden"), false);
        DesignMode design;
        design.setProfile(&f.profile);
        QVERIFY(!design.showHidden());
        QVERIFY(design.editLocked());
    }

    void renderQualityThreeSettings()
    {
        Fixture f;
        const QStringList keys{QStringLiteral("high-quality"), QStringLiteral("high-quality-while-writing"),
                               QStringLiteral("high-quality-while-editing")};
        const QString section = sectionOfKey(f.schema, keys.first());
        QVERIFY(!section.isEmpty());
        for (const QString& key : keys) {
            const QVariantMap entry = entryOf(f.schema, section, key);
            QVERIFY2(!entry.isEmpty(), qPrintable(key));
            QCOMPARE(entry.value(QStringLiteral("type")).toString(), QStringLiteral("bool"));
            QCOMPARE(entry.value(QStringLiteral("store")).toString(),
                     QString::fromLatin1(SettingsSchema::kProfileStore));
        }
    }

    void autoGroupDefaultsOff()
    {
        Fixture f;
        const QString key = QStringLiteral("auto-group-strokes");
        const QString section = sectionOfKey(f.schema, key);
        QVERIFY(!section.isEmpty());
        QCOMPARE(entryOf(f.schema, section, key).value(QStringLiteral("defaultValue")), QVariant(false));
        QCOMPARE(f.schema.value(section, key), QVariant(false));
    }

    void editingQualityDefaultsOff()
    {
        Fixture f;
        for (const QString& key :
             {QStringLiteral("high-quality-while-writing"), QStringLiteral("high-quality-while-editing")}) {
            const QString section = sectionOfKey(f.schema, key);
            QVERIFY2(!section.isEmpty(), qPrintable(key));
            QCOMPARE(entryOf(f.schema, section, key).value(QStringLiteral("defaultValue")), QVariant(false));
            QCOMPARE(f.schema.value(section, key), QVariant(false));
        }
    }

    void defaultPageSize()
    {
        Fixture f;
        const QString key = QStringLiteral("page-size");
        const QString section = sectionOfKey(f.schema, key);
        QVERIFY(!section.isEmpty());
        QCOMPARE(entryOf(f.schema, section, key).value(QStringLiteral("defaultValue")).toString(),
                 QStringLiteral("1024x768"));
        QCOMPARE(f.schema.value(section, key).toString(), QStringLiteral("1024x768"));

        // D6-13 : réglage `defaults/page-size` du profil ; une taille hors des choix est refusée.
        f.schema.setValue(section, key, QStringLiteral("1280x720"));
        QCOMPARE(f.profile.setting(QStringLiteral("defaults"), key, {}).toString(),
                 QStringLiteral("1280x720"));
        f.schema.setValue(section, key, QStringLiteral("777x1"));
        QCOMPARE(f.schema.value(section, key).toString(), QStringLiteral("1280x720"));
    }

    // Renvoi de L6-T28 : la page réseau et ses réglages existent dès la création de la page des paramètres
    // (que Main.qml construit au démarrage), sans navigation ni Loader paresseux. C2 : le schéma reçoit
    // root.profile et root.preferences, aucun identifiant de Main.qml.
    void networkPageBuiltWithTheSettingsPage()
    {
        Fixture f;
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.loadFromModule("ActCraft.Ui", "SettingsPage");
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        const std::unique_ptr<QObject> page(component.createWithInitialProperties(
            {{QStringLiteral("profile"), QVariant::fromValue(&f.profile)},
             {QStringLiteral("preferences"), QVariant::fromValue(&f.preferences)}}));
        QVERIFY(page);
        QObject* network = page->findChild<QObject*>(QStringLiteral("networkPage"));
        QVERIFY(network);
        QVERIFY(network->findChild<NetworkSettings*>());
        auto* schema = page->findChild<SettingsSchema*>(QStringLiteral("settingsSchema"));
        QVERIFY(schema);
        QCOMPARE(schema->profile(), &f.profile);
        QCOMPARE(schema->preferences(), &f.preferences);
    }

private:
    QTemporaryDir m_settingsDir;
};

QTEST_MAIN(TstUiSettingsschema)
#include "tst_ui_settingsschema.moc"
