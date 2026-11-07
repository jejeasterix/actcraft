#include <actcraft/ui/designmode.h>
#include <actcraft/ui/networksettings.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/profilecontroller.h>
#include <actcraft/ui/recorders.h>
#include <actcraft/ui/settingsschema.h>

#include <QJSValue>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
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

// Clés QSettings du lot 7 (A19), lues sur feat/livraison-complete-b, avec le type que leur lecteur relit.
// `tools/ticker/message` (JSON du bandeau, TickerController) ne se règle pas par un champ : pas montré.
struct Lot7Key
{
    const char* key;
    const char* type;
    QVariant defaultValue;
};
const QList<Lot7Key> kLot7Keys{
    {"tools/webHome", "text", QString()},                                 // PresentationCommands
    {"tools/calculator/mode", "int", 0},                                  // CalculatorController, 0 à 2
    {"tools/calculator/program", "text", QString()},                      // CalculatorController
    {"presentation/transition/global", "choice", QStringLiteral("none")}, // TransitionController
    {"presentation/transition/duration", "int", 15},                      // dixièmes, 5 à 30
};

// Catalogue des effets du lot 7 (features/src/transitions.cpp) : « none », « random », puis 16 mouvements.
const QStringList kTransitionEffects{
    QStringLiteral("none"),        QStringLiteral("random"),       QStringLiteral("dissolve"),
    QStringLiteral("corner-peel"), QStringLiteral("box-turn"),     QStringLiteral("card-flip"),
    QStringLiteral("tiles-spin"),  QStringLiteral("fold-half"),    QStringLiteral("slide-right"),
    QStringLiteral("strips-drop"), QStringLiteral("corner-swing"), QStringLiteral("spiral-out"),
    QStringLiteral("turn-reveal"), QStringLiteral("drapes"),       QStringLiteral("star-iris"),
    QStringLiteral("shards"),      QStringLiteral("sweep"),        QStringLiteral("funnel")};

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

QStringList allKeys(const SettingsSchema& schema)
{
    QStringList keys;
    for (const QString& id : sectionIds(schema)) {
        for (const QVariant& entry : schema.entries(id))
            keys << entry.toMap().value(QStringLiteral("key")).toString();
    }
    return keys;
}

std::unique_ptr<QObject> createSettingsPage(QQmlEngine& engine, Fixture& f)
{
    QQmlComponent component(&engine);
    component.loadFromModule("ActCraft.Ui", "SettingsPage");
    if (!component.isReady()) {
        qWarning().noquote() << component.errorString();
        return {};
    }
    return std::unique_ptr<QObject>(component.createWithInitialProperties(
        {{QStringLiteral("profile"), QVariant::fromValue(&f.profile)},
         {QStringLiteral("preferences"), QVariant::fromValue(&f.preferences)}}));
}

// Éléments visuels, délégués de Repeater et contenus de Loader compris.
QQuickItem* findItem(QQuickItem* item, const QString& name)
{
    if (item->objectName() == name)
        return item;
    for (QQuickItem* child : item->childItems()) {
        if (QQuickItem* found = findItem(child, name))
            return found;
    }
    return nullptr;
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

    // Le test qui distingue : chaque réglage du lot 7 porte la clé que son lecteur relit, avec son type et
    // ses bornes ; la valeur passe par QSettings en direct, lue comme le lot 7 la lit (groupe compris).
    // Sous les clés de l'essai 1 (`tools/ticker`, `tools/calculator`, `presentation/transition` en booléen),
    // la première boucle échoue.
    void lot7KeysReused()
    {
        Fixture f;
        for (const Lot7Key& expected : kLot7Keys) {
            const QString key = QString::fromLatin1(expected.key);
            const QString section = sectionOfKey(f.schema, key);
            QVERIFY2(!section.isEmpty(), expected.key);
            const QVariantMap entry = entryOf(f.schema, section, key);
            QCOMPARE(entry.value(QStringLiteral("store")).toString(),
                     QString::fromLatin1(SettingsSchema::kWorkstationStore));
            QCOMPARE(entry.value(QStringLiteral("type")).toString(), QString::fromLatin1(expected.type));
            QCOMPARE(entry.value(QStringLiteral("defaultValue")), expected.defaultValue);
            QCOMPARE(f.schema.value(section, key), expected.defaultValue); // rien d'écrit : le défaut
        }
        // Aucun réglage sur un groupe du lot 7 ni sur le JSON du bandeau.
        for (const QString& key : allKeys(f.schema)) {
            QVERIFY2(!key.startsWith(QLatin1String("tools/ticker")), qPrintable(key));
            QVERIFY2(key != QLatin1String("tools/calculator"), qPrintable(key));
            QVERIFY2(key != QLatin1String("presentation/transition"), qPrintable(key));
        }

        const QString tools = sectionOfKey(f.schema, QStringLiteral("tools/calculator/mode"));
        const QString effects = sectionOfKey(f.schema, QStringLiteral("presentation/transition/global"));
        const QVariantMap mode = entryOf(f.schema, tools, QStringLiteral("tools/calculator/mode"));
        QCOMPARE(mode.value(QStringLiteral("min")).toInt(), 0);
        QCOMPARE(mode.value(QStringLiteral("max")).toInt(), 2);
        const QVariantMap duration =
            entryOf(f.schema, effects, QStringLiteral("presentation/transition/duration"));
        QCOMPARE(duration.value(QStringLiteral("min")).toInt(), 5);
        QCOMPARE(duration.value(QStringLiteral("max")).toInt(), 30);
        QCOMPARE(entryOf(f.schema, effects, QStringLiteral("presentation/transition/global"))
                     .value(QStringLiteral("choices"))
                     .toStringList(),
                 kTransitionEffects);

        // Écrit par le lot 7 : lu par le schéma, au type du réglage.
        QSettings().setValue(QStringLiteral("tools/webHome"), QStringLiteral("https://exemple.org/"));
        QSettings().setValue(QStringLiteral("tools/calculator/mode"), 1);
        QCOMPARE(f.schema.value(tools, QStringLiteral("tools/webHome")),
                 QVariant(QStringLiteral("https://exemple.org/")));
        QCOMPARE(f.schema.value(tools, QStringLiteral("tools/calculator/mode")), QVariant(1));

        // Écrit par le schéma : lu par le lot 7, sous ses clés, bornes et choix du lot 7, sans autre clé.
        const QSet<QString> before = settingsKeys();
        f.schema.setValue(effects, QStringLiteral("presentation/transition/global"),
                          QStringLiteral("dissolve"));
        f.schema.setValue(effects, QStringLiteral("presentation/transition/global"),
                          QStringLiteral("inconnu"));
        f.schema.setValue(effects, QStringLiteral("presentation/transition/duration"), 99);
        f.schema.setValue(tools, QStringLiteral("tools/calculator/mode"), 7);
        f.schema.setValue(tools, QStringLiteral("tools/calculator/program"), QStringLiteral("/usr/bin/bc"));
        QSettings settings;
        settings.beginGroup(QStringLiteral("presentation/transition")); // comme TransitionController
        QCOMPARE(settings.value(QStringLiteral("global")).toString(), QStringLiteral("dissolve"));
        QCOMPARE(settings.value(QStringLiteral("duration")).toInt(), 30);
        settings.endGroup();
        QCOMPARE(settings.value(QStringLiteral("tools/calculator/mode")).toInt(), 2);
        QCOMPARE(settings.value(QStringLiteral("tools/calculator/program")).toString(),
                 QStringLiteral("/usr/bin/bc"));
        QSet<QString> added = settingsKeys();
        added.subtract(before);
        QCOMPARE(added, (QSet<QString>{QStringLiteral("presentation/transition/global"),
                                       QStringLiteral("presentation/transition/duration"),
                                       QStringLiteral("tools/calculator/program")}));
    }

    void profileOrWorkstationStore()
    {
        Fixture f;
        // Clés QSettings existantes : celles du lot 7, de Preferences et de NetworkSettings ; aucune neuve.
        QSet<QString> existing;
        for (const Lot7Key& lot7 : kLot7Keys)
            existing << QString::fromLatin1(lot7.key);
        existing << QString::fromLatin1(Preferences::kLanguage) << QString::fromLatin1(NetworkSettings::kHost)
                 << QString::fromLatin1(NetworkSettings::kPort)
                 << QString::fromLatin1(NetworkSettings::kUser)
                 // Lot 8 (L8-T28) : section « Enregistrements », lue par ui::Recorders.
                 << QString::fromLatin1(actcraft::ui::Recorders::kFolderKey)
                 << QString::fromLatin1(actcraft::ui::Recorders::kSoundFormatKey)
                 << QString::fromLatin1(actcraft::ui::Recorders::kScreenFormatKey);
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
        // Haute qualité hors écriture et édition : vrai par défaut (le rendu courant de l'application).
        QCOMPARE(entryOf(f.schema, section, keys.first()).value(QStringLiteral("defaultValue")),
                 QVariant(true));
        QCOMPARE(f.schema.value(section, keys.first()), QVariant(true));
    }

    // Règles d'ignorance de F-930, défauts du plan (L6-T33, SpellRules) : dans le profil, section `spelling`,
    // clés que L6-T33 lira.
    void spellingRulesDefaults()
    {
        Fixture f;
        const QList<QPair<QString, bool>> rules{
            {QStringLiteral("ignore-capitalized"), false}, {QStringLiteral("ignore-uppercase"), false},
            {QStringLiteral("ignore-digits"), false},      {QStringLiteral("ignore-mixed-case"), false},
            {QStringLiteral("ignore-addresses"), false},   {QStringLiteral("flag-repeated"), true}};
        const QString section = QStringLiteral("spelling");
        QCOMPARE(f.schema.entries(section).size(), rules.size());
        for (const auto& [key, defaultValue] : rules) {
            const QVariantMap entry = entryOf(f.schema, section, key);
            QVERIFY2(!entry.isEmpty(), qPrintable(key));
            QCOMPARE(entry.value(QStringLiteral("type")).toString(), QStringLiteral("bool"));
            QCOMPARE(entry.value(QStringLiteral("store")).toString(),
                     QString::fromLatin1(SettingsSchema::kProfileStore));
            QCOMPARE(entry.value(QStringLiteral("defaultValue")), QVariant(defaultValue));
            QCOMPARE(f.schema.value(section, key), QVariant(defaultValue));
        }
        f.schema.setValue(section, QStringLiteral("flag-repeated"), false);
        QCOMPARE(f.profile.setting(section, QStringLiteral("flag-repeated"), true), QVariant(false));
    }

    // Dossier des ressources partagées : la clé que ResourceLibrary lit déjà (`ressources/partagees` du
    // profil), même magasin.
    void sharedResourcesFolder()
    {
        Fixture f;
        const QString section = QStringLiteral("profile");
        const QString key = QStringLiteral("partagees");
        const QVariantMap entry = entryOf(f.schema, section, key);
        QVERIFY(!entry.isEmpty());
        QCOMPARE(entry.value(QStringLiteral("type")).toString(), QStringLiteral("text"));
        QCOMPARE(entry.value(QStringLiteral("store")).toString(),
                 QString::fromLatin1(SettingsSchema::kProfileStore));
        const QString folder = f.dir.filePath(QStringLiteral("partage"));
        f.schema.setValue(section, key, folder);
        QCOMPARE(f.profile.setting(QStringLiteral("ressources"), key, QString()).toString(), folder);
        QVERIFY(!f.profile.setting(section, key, {}).isValid());
        QCOMPARE(f.schema.value(section, key).toString(), folder);
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

    // « Revenir à l'origine » (ProfileController::resetCurrent : store().reset puis activate du même
    // identifiant) recharge le profil sans changer de profil : l'éditeur relit sa valeur. Sans relecture,
    // la case garde 25 alors que le profil vaut 10.
    void editorsRereadAfterReset()
    {
        Fixture f;
        QQmlEngine engine;
        const std::unique_ptr<QObject> page = createSettingsPage(engine, f);
        QVERIFY(page);
        page->setProperty("currentSection", QStringLiteral("objects"));
        QQuickItem* spin =
            findItem(qobject_cast<QQuickItem*>(page.get()), QStringLiteral("setting-centering-tolerance"));
        QVERIFY(spin);
        QCOMPARE(spin->property("value").toInt(), 10);

        // Écrit hors de l'éditeur (autre schéma, même profil) : relu.
        f.schema.setValue(QStringLiteral("objects"), QStringLiteral("centering-tolerance"), 25);
        QCOMPARE(spin->property("value").toInt(), 25);

        f.profile.saveNow();
        const QString id = f.profile.property("currentId").toString();
        QString error;
        QVERIFY2(f.profile.store().reset(id, error), qPrintable(error));
        QVERIFY(f.profile.activate(id));
        QCOMPARE(f.profile.property("currentId").toString(), id);
        QCOMPARE(f.schema.value(QStringLiteral("objects"), QStringLiteral("centering-tolerance")).toInt(),
                 10);
        QCOMPARE(spin->property("value").toInt(), 10);
    }

    // Une section sans réglage n'est pas montrée (langue avant L6-T36, enregistrements et multimédia avant le
    // lot 8) ; une section qui en a un l'est. Le réseau suit sa plateforme (sectionsOfIncludedFunctionsOnly).
    void emptySectionsHidden()
    {
        Fixture f;
        QQmlEngine engine;
        const std::unique_ptr<QObject> page = createSettingsPage(engine, f);
        QVERIFY(page);
        QVariant shown = page->property("shownSections");
        if (shown.metaType() == QMetaType::fromType<QJSValue>())
            shown = shown.value<QJSValue>().toVariant();
        QStringList ids;
        for (const QVariant& section : shown.toList())
            ids << section.toMap().value(QStringLiteral("id")).toString();
        QVERIFY(ids.contains(QStringLiteral("spelling")));
        for (const QString& id : sectionIds(f.schema)) {
            if (id != QLatin1String("network"))
                QVERIFY2(ids.contains(id) == !f.schema.entries(id).isEmpty(), qPrintable(id));
        }
    }

    // Un passage du focus sans saisie n'écrit rien : aucune clé du poste créée vide.
    void textEditorWritesOnlyAChange()
    {
        Fixture f;
        QQmlEngine engine;
        const std::unique_ptr<QObject> page = createSettingsPage(engine, f);
        QVERIFY(page);
        const QString key = QStringLiteral("tools/webHome");
        page->setProperty("currentSection", sectionOfKey(f.schema, key));
        QQuickItem* field = findItem(qobject_cast<QQuickItem*>(page.get()), QStringLiteral("setting-") + key);
        QVERIFY(field);
        QVERIFY(QMetaObject::invokeMethod(field, "editingFinished"));
        QVERIFY(!QSettings().contains(key));
        field->setProperty("text", QStringLiteral("https://exemple.org/"));
        QVERIFY(QMetaObject::invokeMethod(field, "editingFinished"));
        QCOMPARE(QSettings().value(key).toString(), QStringLiteral("https://exemple.org/"));
    }

private:
    QTemporaryDir m_settingsDir;
};

QTEST_MAIN(TstUiSettingsschema)
#include "tst_ui_settingsschema.moc"
