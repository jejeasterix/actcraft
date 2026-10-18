#include <actcraft/features/pagetools.h>

namespace actcraft::features {

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
