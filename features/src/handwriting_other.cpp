#include <actcraft/features/handwriting.h>

namespace actcraft::features {

// Hors de Windows, aucun moteur d'écriture : makeHandwritingRecognizer() retombe sur « indisponible ».
std::unique_ptr<HandwritingRecognizer> makePlatformHandwritingRecognizer()
{
    return nullptr;
}

// Pas de COM hors de Windows : aucun appartement à ouvrir, rien à fermer.
bool enterRecognitionApartment()
{
    return false;
}

void leaveRecognitionApartment() {}

} // namespace actcraft::features
