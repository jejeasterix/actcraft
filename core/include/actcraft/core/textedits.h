#pragma once

#include <actcraft/core/document.h>
#include <actcraft/core/undo.h>

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

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
    bool m_applied = false; // redo() a trouvé le texte : undo() a quelque chose à défaire
};

// Plusieurs textes remplacés en une étape (L6-P1 : couleur choisie appliquée aux textes sélectionnés).
// Changes : un ObjectChanged par texte trouvé (zone = emprise avant ∪ après), dans l'ordre donné ; objet
// absent ou qui n'est pas un texte : ignoré. Aucun texte trouvé : Changes vide (DocumentController::execute
// ne l'empile pas ; UndoStack::push, lui, l'enregistre).
class SetTextsCommand final : public UndoCommand
{
public:
    SetTextsCommand(std::size_t page, std::vector<std::pair<ObjectId, Text>> texts);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    std::vector<std::pair<ObjectId, Text>> m_after;
    std::vector<std::pair<ObjectId, Text>> m_before; // textes trouvés au dernier redo
};

// Ligne de base de la première ligne, coordonnées de l'objet : celle de la première ligne lue (runs) ; sinon
// celle que la validation a mesurée (Text::firstBaseline, H4-6) ; sinon std::nullopt (l'appelant prend le
// coin inférieur gauche de l'emprise).
std::optional<PointF> textBaseline(const Text& text) noexcept;

} // namespace actcraft::core
