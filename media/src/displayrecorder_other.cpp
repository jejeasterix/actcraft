#include "actcraft/media/displayrecorder.h"

namespace actcraft::media {

// Hors Windows, le module est construit sans moteur d'écran : la commande est absente et l'enregistreur se
// dit indisponible.
std::unique_ptr<DisplayEngine> DisplayRecorder::createScreenEngine()
{
    return nullptr;
}

QString DisplayRecorder::unavailableReason()
{
    return tr("Enregistrement de l'écran : Windows seulement.");
}

} // namespace actcraft::media
