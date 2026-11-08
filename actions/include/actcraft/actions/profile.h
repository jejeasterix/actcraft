#pragma once

#include <actcraft/actions/commandregistry.h>

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <optional>

namespace actcraft::actions {

enum class Dock : std::uint8_t
{
    Left,
    Right,
    Top,
    Bottom,
    Floating,
};
QString dockName(Dock dock); // « left », « right », « top », « bottom », « floating »
std::optional<Dock> dockFromName(const QString& name);

struct PaletteSettings
{
    Dock dock = Dock::Right;
    double floatX = 0; // position flottante, pixels de fenêtre
    double floatY = 0;
    bool collapsed = false;       // F-031 : première rangée seulement
    bool shortcutsHidden = false; // F-032 : barre de raccourcis masquée
    bool pinned = true;           // F-031 : faux = repli au retrait du pointeur
    int swatches = 16;            // F-900 : 8, 16 ou 24
    QJsonObject unknown; // clés de « palette » inconnues de cette version, réécrites telles quelles (H6-12)
};

struct BrowserSettings
{
    bool visible = false;
    QString side = QStringLiteral("left");
    bool pinned = true;
    // Identifiant d'un navigateur de `BrowserPanel.navigators` : « pages », « ressources », « objets », «
    // notes », « proprietes », « actions » ; une valeur inconnue lue d'un fichier devient « pages ».
    QString current = QStringLiteral("pages");
    int width = 240;
    QJsonObject unknown; // clés de « browsers » inconnues de cette version, réécrites telles quelles (H6-12)
};

inline constexpr int kIconPixelsMin = 16;
inline constexpr int kIconPixelsMax = 32;
int clampIconPixels(int pixels) noexcept;

struct Profile
{
    QString id;    // [a-z0-9-]+
    QString label; // français, écrit par nous
    int iconPixels = 24;
    PaletteSettings palette;
    BrowserSettings browsers;
    QStringList extraColors; // « #RRGGBB » ajoutées par l'utilisateur (F-135)
    QStringList recentTools; // F-140, 8 au plus, plus récent d'abord
    QStringList pinned;      // D6-3 : commandes ajoutées à la palette par le profil
    bool fitPage = false;
    bool trashVisible = true;
    QJsonObject shortcutObjects; // F-141 : { "<id>": { "label", "objects" (fragment) } }
    QJsonObject launchers;       // F-920 : écrit par L6-T29
    QJsonObject settings;        // sections de F-930 : { "<section>": { "<clé>": valeur } }
    QJsonObject unknownKeys; // clés de premier niveau inconnues de cette version, réécrites telles quelles
                             // (celles des sous-objets : PaletteSettings::unknown, BrowserSettings::unknown)
};

inline constexpr char kProfileFormat[] = "actcraft-profile/1";

QByteArray profileToJson(const Profile& profile);
// Faux si le JSON est invalide, si « format » manque ou si sa version majeure n'est pas 1 ; `error` dit
// pourquoi.
bool profileFromJson(const QByteArray& json, Profile& profile, QString& error);

// Disposition effective d'un profil pour une plateforme (L11-T27) : ses commandes épinglées, dans l'ordre,
// sans doublon, sans celles que le registre ne déclare pas ou dont `platforms` exclut `platform`.
QStringList layoutForPlatform(const Profile& profile, const CommandRegistry& registry, Platform platform);

} // namespace actcraft::actions
