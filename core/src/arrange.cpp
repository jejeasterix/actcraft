#include <actcraft/core/arrange.h>

namespace actcraft::core {

// Talon L5-T1 : rempli par L5-T5.
std::vector<ObjectId> arrangedOrder(const Page& page, const std::vector<ObjectId>& selected, ArrangeMove move)
{
    static_cast<void>(page);
    static_cast<void>(selected);
    static_cast<void>(move);
    return {};
}

// Talon L5-T1 : rempli par L5-T5.
ReorderObjectsCommand::ReorderObjectsCommand(std::size_t page, std::vector<ObjectId> newOrder)
    : m_page(page), m_newOrder(std::move(newOrder))
{}

// Talon L5-T1 : rempli par L5-T5.
void ReorderObjectsCommand::redo(Document& document, Changes& changes)
{
    static_cast<void>(document);
    static_cast<void>(changes);
    static_cast<void>(m_page);
    static_cast<void>(m_oldOrder);
}

// Talon L5-T1 : rempli par L5-T5.
void ReorderObjectsCommand::undo(Document& document, Changes& changes)
{
    static_cast<void>(document);
    static_cast<void>(changes);
}

// Talon L5-T1 : rempli par L5-T5.
SetLayerCommand::SetLayerCommand(std::size_t page, std::vector<ObjectId> ids, Layer layer)
    : m_page(page), m_ids(std::move(ids)), m_layer(layer)
{}

// Talon L5-T1 : rempli par L5-T5.
void SetLayerCommand::redo(Document& document, Changes& changes)
{
    static_cast<void>(document);
    static_cast<void>(changes);
    static_cast<void>(m_page);
    static_cast<void>(m_layer);
    static_cast<void>(m_before);
    static_cast<void>(m_orderBefore);
}

// Talon L5-T1 : rempli par L5-T5.
void SetLayerCommand::undo(Document& document, Changes& changes)
{
    static_cast<void>(document);
    static_cast<void>(changes);
}

// Talon L5-T1 : rempli par L5-T5.
std::unique_ptr<UndoCommand> makeGroupCommand(const Page& page, std::size_t pageIndex,
                                              const std::vector<ObjectId>& ids)
{
    static_cast<void>(page);
    static_cast<void>(pageIndex);
    static_cast<void>(ids);
    return nullptr;
}

// Talon L5-T1 : rempli par L5-T5.
std::unique_ptr<UndoCommand> makeUngroupCommand(const Page& page, std::size_t pageIndex,
                                                const std::vector<ObjectId>& groups)
{
    static_cast<void>(page);
    static_cast<void>(pageIndex);
    static_cast<void>(groups);
    return nullptr;
}

// Talon L5-T1 : rempli par L5-T5.
ObjectId topLevelGroupOf(const Page& page, ObjectId id)
{
    static_cast<void>(page);
    static_cast<void>(id);
    return 0;
}

// Talon L5-T1 : rempli par L5-T5.
std::vector<ObjectId> expandGroups(const Page& page, const std::vector<ObjectId>& ids)
{
    static_cast<void>(page);
    return ids;
}

} // namespace actcraft::core
