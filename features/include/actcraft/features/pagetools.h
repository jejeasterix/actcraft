#pragma once

#include <actcraft/core/model.h>
#include <actcraft/features/curtain.h>
#include <actcraft/features/spotlight.h>

#include <QRectF>

#include <cstdint>

namespace actcraft::features {

// Outil de présentation d'une page (F-591). Type de features/, valeurs égales à core::PageToolsMode (socle) :
// la phase 2 fournit la conversion depuis core::PagePresentation (addendum 1 : aucun type du socle recréé
// ici).
enum class PageToolChoice : std::uint8_t
{
    Keep = 0,
    Curtain = 1,
    Spotlight = 2,
    Off = 3
};

// Réglages de présentation d'une page vus par les outils (F-591 à F-593).
struct PageToolsSettings
{
    PageToolChoice tools = PageToolChoice::Keep;
    Curtain::Start curtainStart = Curtain::Start::Full; // « complet » quand la page ne dit rien (F-592)
    double curtainDistance = 0;                         // pixels de page depuis le bord de départ
    Spotlight::Shape spotlight = Spotlight::Shape::Off; // Off sous le mode spot : rond (défaut à nous)
    QRectF spotlightFrame;                              // vide : cadre par défaut
};

// État des outils au moment de quitter une page.
struct PresentationState
{
    bool curtain = false;
    Spotlight::Shape spotlight = Spotlight::Shape::Off;
};

// Ce que la page d'arrivée demande aux contrôleurs du rideau et du spot.
struct PageToolsResult
{
    bool curtain = false;
    bool resetCurtain = false; // vrai : refermer le store selon curtainStart et curtainDistance
    Curtain::Start curtainStart = Curtain::Start::Full;
    double curtainDistance = 0;
    Spotlight::Shape spotlight = Spotlight::Shape::Off;
    QRectF spotlightFrame; // vide : garder le cadre courant (comme avant) ou cadre par défaut
};

PageToolsResult resolvePageTools(PresentationState now, const PageToolsSettings& page);

// Réglages du document (core::PagePresentation) vus par les outils. Outil et forme du spot : mêmes valeurs des
// deux côtés (static_assert). Rideau : CurtainMode::Off et Full valent « complet » (F-592), les quatre côtés
// gardent leur côté ; CurtainMode n'a pas les valeurs de Curtain::Start (Full vaut 5 d'un côté, 0 de l'autre).
PageToolsSettings toPageToolsSettings(const core::PagePresentation& page);

} // namespace actcraft::features
