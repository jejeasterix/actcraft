#pragma once
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <memory>
#include <string>

namespace actcraft::core {

enum class FillTarget : std::uint8_t
{
    Nothing,
    ShapeInside,
    Outline,
    Image,
    PageBackground,
};
struct FillHit
{
    FillTarget target = FillTarget::Nothing;
    ObjectId object = 0;
};
inline constexpr int kDefaultImageFillTolerance = 5; // F-235, sur 100

// Cible d'un clic en `point` (page), objet du dessus d'abord, verrouillés compris (D10) ; `tolerance` en
// unités de page.
FillHit fillHitAt(const Page& page, PointF point, double tolerance);
// Une étape ; nullptr pour Nothing et Image.
std::unique_ptr<UndoCommand> makeFillCommand(const Page& page, std::size_t pageIndex, const FillHit& hit,
                                             Argb color);
// Remplace la ressource de l'image par `pngBytes` (clé SHA-256 calculée ici) ; une étape.
std::unique_ptr<UndoCommand> makeImageFillCommand(std::size_t pageIndex, ObjectId image,
                                                  std::string pngBytes);

} // namespace actcraft::core
