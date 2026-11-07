#include <actcraft/core/broadcast.h>

#include <array>
#include <utility>

namespace actcraft::core {

namespace {

// Table des catégories, sur les identifiants seuls (core/ ne connaît pas le registre de actions/). page.copy
// et page.cut y sont sans être restreignables dans commands.json : la diffusion protégée les refuse au titre
// de la copie (H8-8), leur drapeau « restrictable » reste celui du défaut.
constexpr std::array<std::pair<std::string_view, Restriction>, 18> kExact = {{
    {"edit.copy", Restriction::Copy},
    {"edit.cut", Restriction::Copy},
    {"edit.duplicate", Restriction::Copy},
    {"page.copy", Restriction::Copy},
    {"page.cut", Restriction::Copy},
    // Revue de L8-T31 : dupliquer comme edit.duplicate, ou sortir la sélection du document vers un fichier
    // sans règles (bibliothèque, barre de raccourcis) ; drapeaux « restrictable » inchangés.
    {"page.duplicate", Restriction::Copy},
    {"object.drag-copy", Restriction::Copy},
    {"transform.mirror-x", Restriction::Copy},
    {"transform.mirror-y", Restriction::Copy},
    {"resources.add", Restriction::Copy},
    {"shortcutbar.add-selection", Restriction::Copy},
    {"file.save", Restriction::Save},
    {"file.save-as", Restriction::Save},
    {"file.publish", Restriction::Save}, // diffuser écrit un nouveau fichier : c'est enregistrer
    {"file.print", Restriction::Print},
    {"webframe.snapshot", Restriction::Camera},  // image du cadre posée sur la page : une capture
    {"record.screen-full", Restriction::Camera}, // la vidéo de l'écran emporte les pages affichées
    {"record.screen-area", Restriction::Camera},
}};

// Familles par préfixe : toute exportation, toute capture de la caméra (D10, arbitré : « tool.camera- »).
constexpr std::array<std::pair<std::string_view, Restriction>, 2> kPrefixes = {{
    {"export.", Restriction::Save},
    {"tool.camera-", Restriction::Camera},
}};

} // namespace

std::optional<Restriction> restrictionFor(std::string_view commandId) noexcept
{
    for (const auto& [id, restriction] : kExact) {
        if (commandId == id) {
            return restriction;
        }
    }
    for (const auto& [prefix, restriction] : kPrefixes) {
        if (commandId.size() > prefix.size() && commandId.starts_with(prefix)) {
            return restriction;
        }
    }
    return std::nullopt;
}

bool forbids(const BroadcastRules& rules, Restriction restriction) noexcept
{
    // Aucune exception pour l'auteur : les informations de l'auteur ne lèvent aucune interdiction.
    switch (restriction) {
    case Restriction::Copy:
        return rules.noCopy;
    case Restriction::Save:
        return rules.noSave;
    case Restriction::Print:
        return rules.noPrint;
    case Restriction::Camera:
        return rules.noCamera;
    }
    return false;
}

Document published(const Document& source, const BroadcastRules& rules)
{
    Document copy = source;
    copy.setBroadcast(rules);
    return copy;
}

} // namespace actcraft::core
