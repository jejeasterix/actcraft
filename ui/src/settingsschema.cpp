#include <actcraft/ui/settingsschema.h>

#include <QCoreApplication>
#include <QStringList>
#include <QVariantMap>

#include <algorithm>

namespace actcraft::ui {

namespace {

// Libellés à nous, traduisibles (L6-T36) : contexte « SettingsSchema ».
constexpr const char* kContext = "SettingsSchema";

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
    const char* type; // bool, int, text, choice
    QVariant defaultValue;
    QStringList choices;
    int min;
    int max;
    const char* store;
    // Section du profil qui garde le réglage quand ce n'est pas celle du schéma (D6-13 :
    // `defaults/page-size`).
    const char* profileSection;
};

constexpr const char* kProfile = SettingsSchema::kProfileStore;
constexpr const char* kWorkstation = SettingsSchema::kWorkstationStore;

Entry boolean(const char* section, const char* key, const char* label, bool defaultValue,
              const char* store = kProfile)
{
    return {section, key, label, "bool", defaultValue, {}, 0, 0, store, nullptr};
}

Entry integer(const char* section, const char* key, const char* label, int defaultValue, int min, int max)
{
    return {section, key, label, "int", defaultValue, {}, min, max, kProfile, nullptr};
}

Entry text(const char* section, const char* key, const char* label, const char* store)
{
    return {section, key, label, "text", QString(), {}, 0, 0, store, nullptr};
}

Entry choice(const char* section, const char* key, const char* label, const QStringList& choices,
             const char* profileSection)
{
    return {section, key, label, "choice", choices.constFirst(), choices, 0, 0, kProfile, profileSection};
}

QString translated(const char* text)
{
    return QCoreApplication::translate(kContext, text);
}

// Sections de F-930 des fonctions incluses (aucune section du vote). `network` : Windows seulement, éditée
// par NetworkPage (L6-T28) ; `language` (L6-T36) et `spelling` (L6-T33, L6-T34) : déclarées, réglages posés
// par leurs tâches ; `recordings` et `multimedia` : déclarées pour le lot 8.
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

const QList<Entry>& entryTable()
{
    static const QList<Entry> table{
        // Horloge (F-930).
        boolean("clock", "show-digits", QT_TRANSLATE_NOOP("SettingsSchema", "Afficher l'heure en chiffres"),
                true),
        boolean("clock", "second-hand", QT_TRANSLATE_NOOP("SettingsSchema", "Montrer la trotteuse"), true),
        boolean("clock", "twenty-four-hours", QT_TRANSLATE_NOOP("SettingsSchema", "Heure sur 24 heures"),
                false),
        // Effets (F-935) ; `presentation/transition` : clé du lot 7 (A19).
        integer("effects", "spotlight-width", QT_TRANSLATE_NOOP("SettingsSchema", "Largeur du bord du spot"),
                15, 1, 100),
        integer("effects", "curtain-speed", QT_TRANSLATE_NOOP("SettingsSchema", "Vitesse du rideau"), 15, 1,
                100),
        boolean("effects", "presentation/transition",
                QT_TRANSLATE_NOOP("SettingsSchema", "Effet au changement de page"), true, kWorkstation),
        // Objets de la page.
        integer("objects", "centering-tolerance",
                QT_TRANSLATE_NOOP("SettingsSchema", "Tolérance de centrage (pixels)"), 10, 0, 100),
        // Outils : clés du lot 7 (A19), reprises telles quelles.
        text("tools", "tools/ticker", QT_TRANSLATE_NOOP("SettingsSchema", "Texte défilant"), kWorkstation),
        text("tools", "tools/calculator", QT_TRANSLATE_NOOP("SettingsSchema", "Calculatrice"), kWorkstation),
        text("tools", "tools/webHome",
             QT_TRANSLATE_NOOP("SettingsSchema", "Page d'accueil du navigateur web"), kWorkstation),
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
               {QStringLiteral("1024x768"), QStringLiteral("1280x720"), QStringLiteral("1280x800"),
                QStringLiteral("1920x1080")},
               "defaults"),
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
    const QLatin1String type(entry.type);
    if (type == QLatin1String("bool"))
        return value.toBool();
    if (type == QLatin1String("int"))
        return value.toInt();
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
        const bool numeric = qstrcmp(entry.type, "int") == 0;
        list.append(QVariantMap{{QStringLiteral("key"), QString::fromLatin1(entry.key)},
                                {QStringLiteral("label"), translated(entry.label)},
                                {QStringLiteral("type"), QString::fromLatin1(entry.type)},
                                {QStringLiteral("defaultValue"), entry.defaultValue},
                                {QStringLiteral("min"), numeric ? QVariant(entry.min) : QVariant()},
                                {QStringLiteral("max"), numeric ? QVariant(entry.max) : QVariant()},
                                {QStringLiteral("choices"), entry.choices},
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
    if (qstrcmp(entry->type, "int") == 0)
        written = std::clamp(written.toInt(), entry->min, entry->max);
    else if (qstrcmp(entry->type, "choice") == 0 && !entry->choices.contains(written.toString()))
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
    emit settingChanged(section, key);
}

} // namespace actcraft::ui
