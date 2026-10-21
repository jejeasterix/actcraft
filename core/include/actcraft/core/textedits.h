#pragma once

#include <actcraft/core/document.h>
#include <actcraft/core/undo.h>

#include <cstddef>
#include <optional>

namespace actcraft::core {

// Texte remplacé (F-251) : HTML, bornes, lignes lues et sens. Changes : un ObjectChanged, zone = emprise
// avant ∪ emprise après. Objet absent ou qui n'est pas un texte : rien.
class SetTextCommand final : public UndoCommand
{
public:
    SetTextCommand(std::size_t page, ObjectId id, Text text);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    ObjectId m_id;
    Text m_after;
    Text m_before;
};

// Ligne de base de la première ligne, coordonnées de l'objet : celle de la première ligne lue (runs) ; sinon
// std::nullopt (l'appelant prend le coin inférieur gauche de la première ligne qu'il a mesurée, ou de
// l'emprise).
std::optional<PointF> textBaseline(const Text& text) noexcept;

} // namespace actcraft::core
