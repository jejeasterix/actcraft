#include <actcraft/features/screenkeyboard.h>

namespace actcraft::features {

// Hors Windows, aucune frappe n'est injectée dans une autre application : le clavier à l'écran tape dans les
// champs d'ActCraft seulement.
bool sendUnicodeToForeground(const QString&)
{
    return false;
}

} // namespace actcraft::features
