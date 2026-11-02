#pragma once
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <memory>
#include <utility>
#include <vector>

namespace actcraft::core {

enum class ArrangeMove : std::uint8_t
{
    Forward,
    Backward,
    Front,
    Back,
};

// Ordre de la page après le déplacement des objets choisis, couche par couche (D5) ; les membres d'un groupe
// choisi bougent avec lui ; ordre inchangé si rien ne bouge.
std::vector<ObjectId> arrangedOrder(const Page& page, const std::vector<ObjectId>& selected,
                                    ArrangeMove move);

// Réordonne la page en une étape. Changes : pour chaque objet dont le rang change, un ObjectRemoved puis un
// ObjectInserted (zone : son emprise) ; l'inverse à l'annulation ; rien si l'ordre est identique.
class ReorderObjectsCommand final : public UndoCommand
{
public:
    ReorderObjectsCommand(std::size_t page, std::vector<ObjectId> newOrder);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    std::vector<ObjectId> m_newOrder;
    std::vector<ObjectId> m_oldOrder;
};

// Change la couche des objets (F-513) : au sommet de la couche d'arrivée, ordre relatif gardé ; une étape.
class SetLayerCommand final : public UndoCommand
{
public:
    SetLayerCommand(std::size_t page, std::vector<ObjectId> ids, Layer layer);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    std::vector<ObjectId> m_ids;
    Layer m_layer;
    std::vector<std::pair<ObjectId, Layer>> m_before;
    std::vector<ObjectId> m_orderBefore;
};

// Groupes (F-525, D4) par ReplaceObjectsCommand (A6) ; nullptr si moins de 2 objets / aucun groupe.
std::unique_ptr<UndoCommand> makeGroupCommand(const Page& page, std::size_t pageIndex,
                                              const std::vector<ObjectId>& ids);
std::unique_ptr<UndoCommand> makeUngroupCommand(const Page& page, std::size_t pageIndex,
                                                const std::vector<ObjectId>& groups);
ObjectId topLevelGroupOf(const Page& page, ObjectId id);
std::vector<ObjectId> expandGroups(const Page& page, const std::vector<ObjectId>& ids);

} // namespace actcraft::core
