#include <actcraft/ui/settingsschema.h>

#include <QCoreApplication>
#include <QStringList>
#include <QVariantMap>

#include <algorithm>

namespace actcraft::ui {

namespace {

// Libellés à nous, traduisibles (L6-T36) : contexte « SettingsSchema ».
constexpr const char* kContext = "SettingsSchema";

enum class Type
{
    Bool,
    Int,
    Text,
    Choice
};

// Nom publié par entries() : SettingsPage.qml choisit l'éditeur sur ce nom.
QString typeName(Type type)
{
    switch (type) {
    case Type::Bool:
        return QStringLiteral("bool");
    case Type::Int:
        return QStringLiteral("int");
    case Type::Text:
        return QStringLiteral("text");
    case Type::Choice:
        return QStringLiteral("choice");
    }
    return {};
}

// Choix d'un réglage : valeur écrite, libellé montré (nul : la valeur elle-même).
struct Option
{
    const char* value;
    const char* label;
};

struct Section
{
    const char* id;
    const char* label;
    QStringList platforms; // vide : toutes
};

struct Entry
{
    const char* section;
    const char* key;
    const char* label;
    Type type;
    QVariant defaultValue;
    QList<Option> options; // Type::Choice
    int min;
    int max;
    const char* store;
    // Section du profil qui garde le réglage quand ce n'est pas celle du schéma (D6-13 :
    // `defaults/page-size`).
    const char* profileSection;
};

constexpr const char* kProfile = SettingsSchema::kProfileStore;
constexpr const char* kWorkstation = SettingsSchema::kWorkstationStore;

Entry boolean(const char* section, const char* key, const char* label, bool defaultValue)
{
    return {section, key, label, Type::Bool, defaultValue, {}, 0, 0, kProfile, nullptr};
}

Entry integer(const char* section, const char* key, const char* label, int defaultValue, int min, int max,
              const char* store = kProfile)
{
    return {section, key, label, Type::Int, defaultValue, {}, min, max, store, nullptr};
}

Entry text(const char* section, const char* key, const char* label, const char* store,
           const char* profileSection = nullptr)
{
    return {section, key, label, Type::Text, QString(), {}, 0, 0, store, profileSection};
}

// Défaut : le premier choix.
Entry choice(const char* section, const char* key, const char* label, const QList<Option>& options,
             const char* store, const char* profileSection = nullptr)
{
    return {section, key, label, Type::Choice, QString::fromLatin1(options.constFirst().value),
            options, 0,   0,     store,        profileSection};
}

QStringList choiceValues(const Entry& entry)
{
    QStringList values;
    for (const Option& option : entry.options)
        values << QString::fromLatin1(option.value);
    return values;
}

QString translated(const char* text)
{
    return QCoreApplication::translate(kContext, text);
}

// Sections de F-930 des fonctions incluses (aucune section du vote). `network` : Windows seulement, éditée
// par NetworkPage (L6-T28) ; `recordings` et `multimedia` (lot 8) : déclarées sans
// réglage, masquées par SettingsPage.qml tant que leurs tâches ne les remplissent pas.
const QList<Section>& sectionTable()
{
    static const QList<Section> table{
        {"clock", QT_TRANSLATE_NOOP("SettingsSchema", "Horloge"), {}},
        {"effects", QT_TRANSLATE_NOOP("SettingsSchema", "Effets"), {}},
        {"objects", QT_TRANSLATE_NOOP("SettingsSchema", "Objets de la page"), {}},
        {"language", QT_TRANSLATE_NOOP("SettingsSchema", "Langue"), {}},
        {"network", QT_TRANSLATE_NOOP("SettingsSchema", "Réseau"), {QStringLiteral("windows")}},
        {"spelling", QT_TRANSLATE_NOOP("SettingsSchema", "Orthographe"), {}},
        {"tools", QT_TRANSLATE_NOOP("SettingsSchema", "Outils"), {}},
        {"input", QT_TRANSLATE_NOOP("SettingsSchema", "Saisie"), {}},
        {"recordings", QT_TRANSLATE_NOOP("SettingsSchema", "Enregistrements"), {}},
        {"profile", QT_TRANSLATE_NOOP("SettingsSchema", "Profils et bibliothèque"), {}},
        {"document", QT_TRANSLATE_NOOP("SettingsSchema", "Document"), {}},
        {"multimedia", QT_TRANSLATE_NOOP("SettingsSchema", "Multimédia"), {}},
        {"design-mode", QT_TRANSLATE_NOOP("SettingsSchema", "Mode conception"), {}},
    };
    return table;
}

// Effets de TransitionController (lot 7, features/src/transitions.cpp) : il ne relit qu'un identifiant de ce
// catalogue. À reprendre de transitionCatalogue() au merge du lot 7.
const QList<Option>& transitionEffects()
{
    static const QList<Option> effects{
        {"none", QT_TRANSLATE_NOOP("SettingsSchema", "Sans effet")},
        {"random", QT_TRANSLATE_NOOP("SettingsSchema", "Au hasard")},
        {"dissolve", QT_TRANSLATE_NOOP("SettingsSchema", "Fondu")},
        {"corner-peel", QT_TRANSLATE_NOOP("SettingsSchema", "Coin soulevé")},
        {"box-turn", QT_TRANSLATE_NOOP("SettingsSchema", "Boîte qui tourne")},
        {"card-flip", QT_TRANSLATE_NOOP("SettingsSchema", "Carte retournée")},
        {"tiles-spin", QT_TRANSLATE_NOOP("SettingsSchema", "Carreaux")},
        {"fold-half", QT_TRANSLATE_NOOP("SettingsSchema", "Pliage")},
        {"slide-right", QT_TRANSLATE_NOOP("SettingsSchema", "Glissement")},
        {"strips-drop", QT_TRANSLATE_NOOP("SettingsSchema", "Bandes")},
        {"corner-swing", QT_TRANSLATE_NOOP("SettingsSchema", "Pivot d'angle")},
        {"spiral-out", QT_TRANSLATE_NOOP("SettingsSchema", "Tourbillon")},
        {"turn-reveal", QT_TRANSLATE_NOOP("SettingsSchema", "Dévoilement tournant")},
        {"drapes", QT_TRANSLATE_NOOP("SettingsSchema", "Tentures")},
        {"star-iris", QT_TRANSLATE_NOOP("SettingsSchema", "Étoile")},
        {"shards", QT_TRANSLATE_NOOP("SettingsSchema", "Éclats")},
        {"sweep", QT_TRANSLATE_NOOP("SettingsSchema", "Balayage")},
        {"funnel", QT_TRANSLATE_NOOP("SettingsSchema", "Entonnoir")},
    };
    return effects;
}

const QList<Entry>& entryTable()
{
    static const QList<Entry> table{
        // Horloge (F-930).
        boolean("clock", "show-digits", QT_TRANSLATE_NOOP("SettingsSchema", "Afficher l'heure en chiffres"),
                true),
        boolean("clock", "second-hand", QT_TRANSLATE_NOOP("SettingsSchema", "Montrer la trotteuse"), true),
        boolean("clock", "twenty-four-hours", QT_TRANSLATE_NOOP("SettingsSchema", "Heure sur 24 heures"),
                false),
        // Effets (F-935) ; transition de page : clés et bornes de TransitionController (lot 7, A19).
        integer("effects", "spotlight-width", QT_TRANSLATE_NOOP("SettingsSchema", "Largeur du bord du spot"),
                15, 1, 100),
        integer("effects", "curtain-speed", QT_TRANSLATE_NOOP("SettingsSchema", "Vitesse du rideau"), 15, 1,
                100),
        choice("effects", "presentation/transition/global",
               QT_TRANSLATE_NOOP("SettingsSchema", "Effet au changement de page"), transitionEffects(),
               kWorkstation),
        integer("effects", "presentation/transition/duration",
                QT_TRANSLATE_NOOP("SettingsSchema", "Durée de l'effet (dixièmes de seconde)"), 15, 5, 30,
                kWorkstation),
        // Langue (L6-T36) : clé de Preferences, appliquée au redémarrage ; défaut : la langue du système.
        choice("language", "workstation/language",
               QT_TRANSLATE_NOOP("SettingsSchema", "Langue de l'interface"),
               {{"", QT_TRANSLATE_NOOP("SettingsSchema", "Langue du système")},
                {"fr", QT_TRANSLATE_NOOP("SettingsSchema", "Français")}},
               kWorkstation),
        // Objets de la page.
        integer("objects", "centering-tolerance",
                QT_TRANSLATE_NOOP("SettingsSchema", "Tolérance de centrage (pixels)"), 10, 0, 100),
        // Outils : clés du lot 7 (A19), au type de leur lecteur (PresentationCommands, CalculatorController).
        // `tools/ticker/message`, JSON du bandeau (TickerController), ne se règle pas par un champ : pas
        // montré.
        text("tools", "tools/webHome",
             QT_TRANSLATE_NOOP("SettingsSchema", "Page d'accueil du navigateur web"), kWorkstation),
        integer("tools", "tools/calculator/mode",
                QT_TRANSLATE_NOOP("SettingsSchema",
                                  "Calculatrice : 0 intégrée, 1 celle du système, 2 le programme ci-dessous"),
                0, 0, 2, kWorkstation),
        text("tools", "tools/calculator/program",
             QT_TRANSLATE_NOOP("SettingsSchema", "Programme de calculatrice"), kWorkstation),
        // Orthographe (F-930) : règles d'ignorance de L6-T33 (SpellRules), défauts du plan.
        boolean("spelling", "ignore-capitalized",
                QT_TRANSLATE_NOOP("SettingsSchema", "Ignorer les mots à majuscule initiale"), false),
        boolean("spelling", "ignore-uppercase",
                QT_TRANSLATE_NOOP("SettingsSchema", "Ignorer les mots en majuscules"), false),
        boolean("spelling", "ignore-digits",
                QT_TRANSLATE_NOOP("SettingsSchema", "Ignorer les mots qui contiennent des chiffres"), false),
        boolean("spelling", "ignore-mixed-case",
                QT_TRANSLATE_NOOP("SettingsSchema", "Ignorer les mots à casse mixte"), false),
        boolean("spelling", "ignore-addresses",
                QT_TRANSLATE_NOOP("SettingsSchema", "Ignorer les adresses web et de courriel"), false),
        boolean("spelling", "flag-repeated", QT_TRANSLATE_NOOP("SettingsSchema", "Signaler les mots répétés"),
                true),
        // Enregistrements (F-292, F-293, L8-T28) : lus par ui::Recorders. Dossier vide : le dossier
        // temporaire du document ; formats : ceux que les moteurs écrivent aujourd'hui.
        text("recordings", "recordings/folder",
             QT_TRANSLATE_NOOP("SettingsSchema", "Dossier des enregistrements"), kWorkstation),
        choice("recordings", "recordings/sound-format", QT_TRANSLATE_NOOP("SettingsSchema", "Format du son"),
               {{"wav", QT_TRANSLATE_NOOP("SettingsSchema", "WAV")}}, kWorkstation),
        choice("recordings", "recordings/screen-format",
               QT_TRANSLATE_NOOP("SettingsSchema", "Format de la vidéo"),
               {{"mp4", QT_TRANSLATE_NOOP("SettingsSchema", "MP4")}}, kWorkstation),
        // Profils et bibliothèque : `ressources/partagees` du profil, que ResourceLibrary lit (vide : le
        // dossier partagé du système).
        text("profile", "partagees", QT_TRANSLATE_NOOP("SettingsSchema", "Dossier des ressources partagées"),
             kProfile, "ressources"),
        // Saisie : P10 du lot 10 (défaut de la spécification ; aucun lecteur au lot 6, publié pour le lot
        // 11).
        boolean("input", "auto-group-strokes",
                QT_TRANSLATE_NOOP("SettingsSchema", "Regrouper les traits successifs"), false),
        // Document : P2 du lot 10 (D14) et E4 (D6-13 : `defaults/page-size` du profil).
        boolean("document", "high-quality", QT_TRANSLATE_NOOP("SettingsSchema", "Rendu en haute qualité"),
                true),
        boolean("document", "high-quality-while-writing",
                QT_TRANSLATE_NOOP("SettingsSchema", "Haute qualité pendant l'écriture"), false),
        boolean("document", "high-quality-while-editing",
                QT_TRANSLATE_NOOP("SettingsSchema", "Haute qualité pendant le déplacement des objets"),
                false),
        choice("document", "page-size", QT_TRANSLATE_NOOP("SettingsSchema", "Taille d'une nouvelle page"),
               {{"1024x768", nullptr}, {"1280x720", nullptr}, {"1280x800", nullptr}, {"1920x1080", nullptr}},
               kProfile, "defaults"),
        // Mode conception (L6-T22) : clés et section `design-mode` de DesignMode, tous vrais par défaut.
        boolean("design-mode", "editLocked",
                QT_TRANSLATE_NOOP("SettingsSchema", "Déplacer les objets verrouillés"), true),
        boolean("design-mode", "showHidden",
                QT_TRANSLATE_NOOP("SettingsSchema", "Montrer les objets masqués"), true),
        boolean("design-mode", "highlightActions",
                QT_TRANSLATE_NOOP("SettingsSchema", "Signaler les objets qui portent une action"), true),
        boolean("design-mode", "disableActions", QT_TRANSLATE_NOOP("SettingsSchema", "Suspendre les actions"),
                true),
        boolean("design-mode", "disableRestrictions",
                QT_TRANSLATE_NOOP("SettingsSchema", "Suspendre les restrictions"), true),
        boolean("design-mode", "disableBlockers",
                QT_TRANSLATE_NOOP("SettingsSchema", "Suspendre les obstacles"), true),
        boolean("design-mode", "disableSnap", QT_TRANSLATE_NOOP("SettingsSchema", "Couper l'accrochage"),
                true),
        boolean("design-mode", "disableDesktop", QT_TRANSLATE_NOOP("SettingsSchema", "Suspendre le bureau"),
                true),
        boolean("design-mode", "showTips", QT_TRANSLATE_NOOP("SettingsSchema", "Montrer les astuces"), true),
    };
    return table;
}

const Entry* find(const QString& section, const QString& key)
{
    const QList<Entry>& table = entryTable();
    const auto it = std::find_if(table.cbegin(), table.cend(), [&](const Entry& entry) {
        return section == QLatin1String(entry.section) && key == QLatin1String(entry.key);
    });
    return it == table.cend() ? nullptr : &*it;
}

bool isProfileStore(const Entry& entry)
{
    return qstrcmp(entry.store, kProfile) == 0;
}

QString profileSectionOf(const Entry& entry)
{
    return QString::fromLatin1(entry.profileSection ? entry.profileSection : entry.section);
}

// Valeur ramenée au type du réglage : le JSON du profil rend des doubles, QSettings des chaînes.
QVariant normalized(const Entry& entry, const QVariant& value)
{
    switch (entry.type) {
    case Type::Bool:
        return value.toBool();
    case Type::Int:
        return value.toInt();
    case Type::Text:
    case Type::Choice:
        break;
    }
    return value.toString();
}

} // namespace

SettingsSchema::SettingsSchema(QObject* parent) : QObject(parent) {}

ProfileController* SettingsSchema::profile() const
{
    return m_profile;
}

void SettingsSchema::setProfile(ProfileController* profile)
{
    if (m_profile == profile)
        return;
    m_profile = profile;
    emit profileChanged();
}

Preferences* SettingsSchema::preferences() const
{
    return m_preferences;
}

void SettingsSchema::setPreferences(Preferences* preferences)
{
    if (m_preferences == preferences)
        return;
    m_preferences = preferences;
    emit preferencesChanged();
}

QVariantList SettingsSchema::sections() const
{
    QVariantList list;
    for (const Section& section : sectionTable()) {
        list.append(QVariantMap{{QStringLiteral("id"), QString::fromLatin1(section.id)},
                                {QStringLiteral("label"), translated(section.label)},
                                {QStringLiteral("platforms"), section.platforms}});
    }
    return list;
}

QVariantList SettingsSchema::entries(const QString& section) const
{
    QVariantList list;
    for (const Entry& entry : entryTable()) {
        if (section != QLatin1String(entry.section))
            continue;
        const bool numeric = entry.type == Type::Int;
        QStringList choiceLabels;
        for (const Option& option : entry.options)
            choiceLabels << (option.label ? translated(option.label) : QString::fromLatin1(option.value));
        list.append(QVariantMap{{QStringLiteral("key"), QString::fromLatin1(entry.key)},
                                {QStringLiteral("label"), translated(entry.label)},
                                {QStringLiteral("type"), typeName(entry.type)},
                                {QStringLiteral("defaultValue"), entry.defaultValue},
                                {QStringLiteral("min"), numeric ? QVariant(entry.min) : QVariant()},
                                {QStringLiteral("max"), numeric ? QVariant(entry.max) : QVariant()},
                                {QStringLiteral("choices"), choiceValues(entry)},
                                {QStringLiteral("choiceLabels"), choiceLabels},
                                {QStringLiteral("store"), QString::fromLatin1(entry.store)}});
    }
    return list;
}

QVariant SettingsSchema::value(const QString& section, const QString& key) const
{
    const Entry* entry = find(section, key);
    if (!entry)
        return {};
    QVariant stored;
    if (isProfileStore(*entry)) {
        if (m_profile)
            stored = m_profile->setting(profileSectionOf(*entry), key, {});
    } else if (m_preferences) {
        stored = m_preferences->value(key); // la clé de son propriétaire, telle quelle
    }
    return stored.isValid() ? normalized(*entry, stored) : entry->defaultValue;
}

void SettingsSchema::setValue(const QString& section, const QString& key, const QVariant& value)
{
    const Entry* entry = find(section, key);
    if (!entry)
        return;
    QVariant written = normalized(*entry, value);
    if (entry->type == Type::Int)
        written = std::clamp(written.toInt(), entry->min, entry->max);
    else if (entry->type == Type::Choice && !choiceValues(*entry).contains(written.toString()))
        return;
    if (isProfileStore(*entry)) {
        if (!m_profile)
            return;
        m_profile->setSetting(profileSectionOf(*entry), key, written);
    } else {
        if (!m_preferences)
            return;
        m_preferences->setValue(key, written);
    }
}

} // namespace actcraft::ui
