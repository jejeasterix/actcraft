#pragma once
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <utility>
#include <vector>

namespace actcraft::core {

enum class AlignMode : std::uint8_t
{
    Left,
    CenterX,
    Right,
    Top,
    CenterY,
    Bottom,
    Width,
    Height,
    Angle,
    All,
};
enum class FitMode : std::uint8_t
{
    Width,
    Height,
    Page,
    OriginalSize,
};
enum class Axis : std::uint8_t
{
    X, // haut / bas
    Y, // gauche / droite
};
using TransformList = std::vector<std::pair<ObjectId, Transform>>; // transformation absolue de chaque objet

// Pose à chaque objet sa transformation, en une étape (aligner, ajuster, inverser, symétries) ; objets
// absents ignorés. Changes : un ObjectChanged par objet modifié (zone : union des emprises avant et après).
class TransformEachCommand final : public UndoCommand
{
public:
    TransformEachCommand(std::size_t page, TransformList transforms);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    TransformList m_transforms;
    TransformList m_before;
};

// Transformations à passer à TransformEachCommand ; objets verrouillés et inchangés absents ; moins de 2
// objets non verrouillés : vide (D6).
TransformList alignTransforms(const Page& page, const std::vector<ObjectId>& ids, AlignMode mode);
TransformList fitTransforms(const Page& page, const std::vector<ObjectId>& ids, FitMode mode); // F-536
TransformList flipTransforms(const Page& page, const std::vector<ObjectId>& ids, Axis axis); // F-537 inverser
TransformList originFlipTransforms(const Page& page, const std::vector<ObjectId>& ids, Axis axis,
                                   PointF origin);
// F-537 projeter : copies miroir au-dessus (X) ou à gauche (Y), bord contre bord ; à passer à preparePaste(…,
// 0, 0).
std::vector<PageObject> mirrorCopies(const Page& page, const std::vector<ObjectId>& ids, Axis axis);

} // namespace actcraft::core
