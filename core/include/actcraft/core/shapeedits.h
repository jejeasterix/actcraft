#pragma once

#include <actcraft/core/document.h>
#include <actcraft/core/undo.h>

#include <cstddef>
#include <vector>

namespace actcraft::core {

// Point intelligent déplacé (F-242) : paramètres remplacés et géométrie reconstruite. Changes : un
// ObjectChanged, zone = emprise avant ∪ emprise après. Objet absent ou sans préréglage : rien.
class SetShapeParametersCommand final : public UndoCommand
{
public:
    SetShapeParametersCommand(std::size_t page, ObjectId id, std::vector<double> parameters);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    ObjectId m_id;
    std::vector<double> m_after;
    std::vector<double> m_before;
};

} // namespace actcraft::core
