#pragma once
// Diffusion protégée (F-680, S18) : règles d'une copie diffusée et catégorie de chaque commande qu'elles
// touchent. Courtoisie sur un format ouvert (N1-A4) : aucun chiffrement.
#include <actcraft/core/document.h>
#include <actcraft/core/model.h>

#include <optional>
#include <string_view>

namespace actcraft::core {

enum class Restriction
{
    Copy,
    Save,
    Print,
    Camera
};

// Catégorie d'une commande restreignable ; nullopt : la diffusion protégée ne la touche pas. Fonction pure
// sur l'identifiant : elle ne lit pas le drapeau « restrictable » du registre (core/ ne connaît pas
// actions/), et classe aussi page.copy et page.cut sous Copy, qui ne sont pas restreignables (H8-8).
std::optional<Restriction> restrictionFor(std::string_view commandId) noexcept;

// Vrai si les règles interdisent cette catégorie ; aucune exception pour l'auteur.
bool forbids(const BroadcastRules& rules, Restriction restriction) noexcept;

// Copie de la source qui porte les règles ; la source n'est pas modifiée.
Document published(const Document& source, const BroadcastRules& rules);

} // namespace actcraft::core
