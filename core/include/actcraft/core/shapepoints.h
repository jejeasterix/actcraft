#pragma once
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <optional>
#include <vector>

namespace actcraft::core {

// Points de forme (F-527, D13) en page : sommets puis milieux des côtés, chemin par chemin ; vide hors Shape.
std::vector<PointF> shapePoints(const PageObject& object);

// Déplace le point `index` en `to` (page), une étape ; la forme devient un polygone ; objet verrouillé : sans
// effet.
class MoveShapePointCommand final : public UndoCommand
{
public:
    MoveShapePointCommand(std::size_t page, ObjectId shape, std::size_t index, PointF to);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    ObjectId m_shape;
    std::size_t m_index;
    PointF m_to;
    std::optional<PageObject> m_before;
};

} // namespace actcraft::core
