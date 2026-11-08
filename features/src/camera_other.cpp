#include <actcraft/features/camera.h>

namespace actcraft::features {

// Hors de Windows : aucune liste des fenêtres ; la capture d'une fenêtre prend l'écran entier.
std::vector<QRect> topLevelWindows()
{
    return {};
}

} // namespace actcraft::features
