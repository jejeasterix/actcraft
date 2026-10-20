#include <actcraft/features/pagetools.h>

namespace actcraft::features {

namespace {

using CoreTools = core::PageToolsMode;
using CoreShape = core::SpotlightShape;

// Mêmes valeurs des deux côtés : la conversion est un transtypage, vérifié valeur par valeur.
static_assert(static_cast<int>(PageToolChoice::Keep) == static_cast<int>(CoreTools::Keep));
static_assert(static_cast<int>(PageToolChoice::Curtain) == static_cast<int>(CoreTools::Curtain));
static_assert(static_cast<int>(PageToolChoice::Spotlight) == static_cast<int>(CoreTools::Spotlight));
static_assert(static_cast<int>(PageToolChoice::Off) == static_cast<int>(CoreTools::Off));
static_assert(static_cast<int>(Spotlight::Shape::Off) == static_cast<int>(CoreShape::Off));
static_assert(static_cast<int>(Spotlight::Shape::Circle) == static_cast<int>(CoreShape::Circle));
static_assert(static_cast<int>(Spotlight::Shape::Square) == static_cast<int>(CoreShape::Square));
static_assert(static_cast<int>(Spotlight::Shape::SolidCircle) == static_cast<int>(CoreShape::SolidCircle));
static_assert(static_cast<int>(Spotlight::Shape::SolidSquare) == static_cast<int>(CoreShape::SolidSquare));

// Côté de départ du store : un tableau à la main, jamais un transtypage (CurtainMode::Full vaut 5, hors de
// Curtain::Start).
Curtain::Start startOf(core::CurtainMode mode) noexcept
{
    switch (mode) {
    case core::CurtainMode::Top:
        return Curtain::Start::Top;
    case core::CurtainMode::Bottom:
        return Curtain::Start::Bottom;
    case core::CurtainMode::Left:
        return Curtain::Start::Left;
    case core::CurtainMode::Right:
        return Curtain::Start::Right;
    case core::CurtainMode::Off: // F-592 : un rideau de page sans mode s'ouvre complet
    case core::CurtainMode::Full:
        break;
    }
    return Curtain::Start::Full;
}

} // namespace

PageToolsSettings toPageToolsSettings(const core::PagePresentation& page)
{
    PageToolsSettings s;
    s.tools = static_cast<PageToolChoice>(static_cast<int>(page.tools));
    s.curtainStart = startOf(page.curtain);
    s.curtainDistance = page.curtainDistance;
    s.spotlight = static_cast<Spotlight::Shape>(static_cast<int>(page.spotlight));
    const core::RectF& f = page.spotlightFrame;
    s.spotlightFrame = QRectF(f.x, f.y, f.width, f.height);
    return s;
}

PageToolsResult resolvePageTools(PresentationState now, const PageToolsSettings& page)
{
    PageToolsResult r;
    switch (page.tools) {
    case PageToolChoice::Keep: // F-591 « comme avant » : l'état de la page quittée continue
        r.curtain = now.curtain;
        r.spotlight = now.spotlight;
        break;
    case PageToolChoice::Curtain:
        r.curtain = true;
        r.resetCurtain = true;
        r.curtainStart = page.curtainStart;
        r.curtainDistance = page.curtainDistance;
        break;
    case PageToolChoice::Spotlight:
        r.spotlight = page.spotlight == Spotlight::Shape::Off ? Spotlight::Shape::Circle : page.spotlight;
        r.spotlightFrame = page.spotlightFrame;
        break;
    case PageToolChoice::Off:
        break;
    }
    return r;
}

} // namespace actcraft::features
