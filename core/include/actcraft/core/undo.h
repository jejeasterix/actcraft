#pragma once

#include <actcraft/core/model.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

namespace actcraft::core {

class Document;

// Ce qu'une commande a changé : le cache de la page affichée (DA11) et l'interface s'en servent.
struct Change
{
    enum class Kind : std::uint8_t
    {
        ObjectInserted,
        ObjectRemoved,
        ObjectChanged,
        PageInserted,
        PageRemoved,
        PageChanged,
    };
    Kind kind;
    std::size_t page;
    ObjectId object; // 0 pour un changement de page
    RectF area;      // zone touchée en coordonnées de page ; vide = toute la page
};

using Changes = std::vector<Change>;

// Une modification du document (décision 16). Ne garde ni pointeur ni référence vers les objets d'une page :
// un indice de page et un identifiant d'objet.
class UndoCommand
{
public:
    virtual ~UndoCommand() = default;
    virtual void redo(Document& document, Changes& changes) = 0;
    virtual void undo(Document& document, Changes& changes) = 0;
    // Deux commandes consécutives de même famille (≥ 0) peuvent fusionner : glissements continus (lot 3).
    virtual int mergeId() const noexcept { return -1; }
    // Absorbe `other`, déjà exécutée ; vrai si la fusion a eu lieu.
    virtual bool mergeWith(const UndoCommand& other)
    {
        static_cast<void>(other);
        return false;
    }
};

// Pile d'annulation d'un document (décision 16, N2-1 : 500 étapes). Toute modification du document passe par
// push() ; l'état propre suit le dernier enregistrement.
class UndoStack
{
public:
    static constexpr std::size_t kDefaultLimit = 500;

    explicit UndoStack(std::size_t limit = kDefaultLimit); // 0 traité comme 1

    // Exécute la commande, efface les commandes annulées, fusionne si possible, borne à limit().
    Changes push(Document& document, std::unique_ptr<UndoCommand> command);
    // Comme push(), pour une commande que l'appelant a déjà exécutée une fois : elle n'est pas rejouée.
    void pushExecuted(std::unique_ptr<UndoCommand> command);
    Changes undo(Document& document); // rien si !canUndo()
    Changes redo(Document& document); // rien si !canRedo()

    bool canUndo() const noexcept;
    bool canRedo() const noexcept;
    std::size_t count() const noexcept;
    std::size_t index() const noexcept; // commandes exécutées (0 … count())
    std::size_t limit() const noexcept;

    bool isClean() const noexcept;
    void setClean() noexcept; // l'état courant est celui du fichier
    void clear() noexcept;    // pile vide, état propre

private:
    void record(std::unique_ptr<UndoCommand> command); // efface les annulées, fusionne, borne
    static constexpr std::size_t kNoClean = std::numeric_limits<std::size_t>::max();

    std::vector<std::unique_ptr<UndoCommand>> m_commands;
    std::size_t m_index = 0;
    std::size_t m_clean = 0;
    std::size_t m_limit;
};

} // namespace actcraft::core
