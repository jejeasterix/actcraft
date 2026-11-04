#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace actcraft::actions {

// Plateformes où une commande existe (DA10) : un bit chacune.
enum class Platform : std::uint8_t
{
    Windows = 1,
    Android = 2,
    MacOS = 4,
    Linux = 8,
};
using Platforms = std::uint8_t;
inline constexpr Platforms kAllPlatforms = 0x0F;

// « windows », « android », « macos », « linux » → bit ; 0 si le nom est inconnu.
Platforms platformBit(const QString& name) noexcept;
// Plateforme d'exécution, lue par QSysInfo::productType() (pas de #ifdef, DA10).
Platform currentPlatform();
QString platformName(Platform platform);

enum class ArgumentKind : std::uint8_t
{
    None,   // aucun argument (tout argument est ignoré)
    Color,  // « #RRGGBB » ou « #AARRGGBB »
    Number, // réel fini
    Url,    // texte non vide formant une URL valide : chemin local ou content:// (décisions 17 et 20)
    Text,   // texte non vide (nom d'un effet, d'un mode)
};

struct CommandInfo
{
    QString id;       // anglais, à points : « page.next »
    QString label;    // français, écrit par nous
    QString icon;     // nom d'icône sans extension (ui/icons/<icon>.svg) ; vide = sans icône
    QString shortcut; // QKeySequence::PortableText ; vide = aucun
    bool checkable = false;
    Platforms platforms = kAllPlatforms;
    ArgumentKind argument = ArgumentKind::None;
    // Vrai : la commande se déclenche aussi sans argument (QVariant invalide) — dialogue au lieu d'une URL
    // (décision 22). Un argument présent doit rester valide.
    bool argumentOptional = false;
    // Amendement 1, § 1 : le mode classe protégé peut la griser (F-298) ; une diffusion protégée peut
    // l'interdire (F-680, lot 8). Défaut sûr : verrouillable, non restreignable.
    bool lockable = true;
    bool restrictable = false;
};

bool argumentIsValid(ArgumentKind kind, const QVariant& argument);

// Registre central des commandes nommées (DA1, décision 20). Sans interface : QtCore seulement.
class CommandRegistry
{
public:
    using Handler = std::function<void(const QVariant& argument)>;

    // Fichier « actcraft-commands/1 ». Tout ou rien : en cas d'erreur, rien n'est déclaré et `error` dit
    // pourquoi (« commande 2 (c.d) : label manquant »).
    bool loadDeclarations(const QByteArray& json, QString& error);
    // Faux si l'identifiant est vide ou déjà déclaré.
    bool declare(const CommandInfo& info);
    const CommandInfo* find(const QString& id) const;
    QStringList ids() const; // ordre de déclaration
    bool isAvailable(const QString& id, Platform platform) const;

    void setHandler(const QString& id, Handler handler);
    bool hasHandler(const QString& id) const;
    void setEnabled(const QString& id, bool enabled);
    // Vrai par défaut ; faux pour une commande inconnue, et pour une commande verrouillable tant que le
    // verrou de classe est posé, quel que soit setEnabled().
    bool isEnabled(const QString& id) const;
    bool isOwnEnabled(const QString& id) const; // état posé par setEnabled, sans le verrou ; faux si inconnue
    void setChecked(const QString& id, bool checked);
    bool isChecked(const QString& id) const; // faux par défaut

    // Verrou de classe (D6-7) : tant qu'il est posé, une commande verrouillable est désactivée et refusée par
    // trigger(), quel que soit setEnabled() ; l'état propre de chaque commande est gardé à part.
    void setLocked(bool locked) noexcept { m_locked = locked; }
    bool isLocked() const noexcept { return m_locked; }

    // Faux, sans rien appeler, si la commande est inconnue, absente de `platform`, désactivée (au sens
    // d'isEnabled : verrou compris), sans gestionnaire, ou si l'argument ne convient pas (un argument absent
    // convient à une commande dont l'argument est facultatif). Le gestionnaire est copié avant l'appel : il
    // peut modifier le registre.
    bool trigger(const QString& id, const QVariant& argument, Platform platform) const;

private:
    struct Entry
    {
        CommandInfo info;
        Handler handler;
        bool enabled = true;
        bool checked = false;
    };
    Entry* entry(const QString& id);
    const Entry* entry(const QString& id) const;

    std::vector<Entry> m_entries;
    QHash<QString, std::size_t> m_index;
    bool m_locked = false;
};

// Dossier des données intégrées du module (« :/actcraft/actions »), ressources initialisées.
QString dataResourceDir();
// Contenu de :/actcraft/actions/commands.json (vide si la ressource manque).
QByteArray defaultCommandsJson();

} // namespace actcraft::actions
