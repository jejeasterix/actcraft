#pragma once

// Zones de page (F-603, D11, L4-T27) : bornes et commande de réglage. Les séparations des cadres sont
// dessinées en surcouche par l'interface (L4-T28) : aucun pixel du cache ne dépend des zones, et elles
// n'attirent rien (aucun point d'accrochage, H4-21).
#include <actcraft/core/document.h>
#include <actcraft/core/undo.h>

#include <cstddef>
#include <cstdint>

namespace actcraft::core {

inline constexpr std::int32_t kMaxFramesAcross = 40; // cadres en largeur (D11)
inline constexpr std::int32_t kMaxFramesDown = 53;   // cadres en hauteur (D11)

inline constexpr double kMinFrameSide =
    500; // H4-20 : minimum vu dans l'original (L4-O2), ni maximum ni pas cherchés

// Zones ramenées dans leurs bornes : nombres de cadres de 0 à kMaxFramesAcross et de 0 à kMaxFramesDown ;
// taille d'un cadre négative ou non finie ramenée à 0 (non donnée), non nulle portée à kMinFrameSide au
// moins.
PageZones clampZones(PageZones zones) noexcept;

// Zones de la page remplacées (F-603), bornées par clampZones. Une étape. Changes : PageSettingsChanged
// (ignoré par le cache, L0-S3 ; contrat A13 : les séparations sont dessinées en surcouche).
class SetPageZonesCommand final : public UndoCommand
{
public:
    SetPageZonesCommand(std::size_t page, PageZones zones);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    PageZones m_after;
    PageZones m_before;
};

} // namespace actcraft::core
