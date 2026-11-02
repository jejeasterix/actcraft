#include <actcraft/core/clipboardedits.h>

#include <utility>

namespace actcraft::core {

// Talon L5-T1 : rempli par L5-T4.
std::vector<PageObject> closureForCopy(const Page& page, const std::vector<ObjectId>& ids)
{
    static_cast<void>(page);
    static_cast<void>(ids);
    return {};
}

// Talon L5-T1 : rempli par L5-T4.
std::vector<PageObject> preparePaste(std::vector<PageObject> objects, double dx, double dy)
{
    static_cast<void>(dx);
    static_cast<void>(dy);
    return objects;
}

// Talon L5-T1 : rempli par L5-T4.
std::vector<PageObject> preparePasteOnto(std::vector<PageObject> objects, const PageObject& target)
{
    static_cast<void>(target);
    return objects;
}

// Talon L5-T1 : rempli par L5-T4.
PasteObjectsCommand::PasteObjectsCommand(std::size_t page, std::vector<PageObject> prepared) : m_page(page)
{
    static_cast<void>(prepared);
}

// Talon L5-T1 : rempli par L5-T4.
void PasteObjectsCommand::redo(Document& document, Changes& changes)
{
    static_cast<void>(document);
    static_cast<void>(changes);
    static_cast<void>(m_page);
    static_cast<void>(m_inner);
}

// Talon L5-T1 : rempli par L5-T4.
void PasteObjectsCommand::undo(Document& document, Changes& changes)
{
    static_cast<void>(document);
    static_cast<void>(changes);
}

// Talon L5-T1 : rempli par L5-T4.
const std::vector<ObjectId>& PasteObjectsCommand::addedIds() const noexcept
{
    return m_ids;
}

// Talon L5-T1 : rempli par L5-T4.
std::unique_ptr<UndoCommand> makeCutCommand(const Page& page, std::size_t pageIndex,
                                            const std::vector<ObjectId>& ids)
{
    static_cast<void>(page);
    static_cast<void>(pageIndex);
    static_cast<void>(ids);
    return nullptr;
}

// Talon L5-T1 : rempli par L5-T4.
std::unique_ptr<PasteObjectsCommand> makeDuplicateCommand(const Page& page, std::size_t pageIndex,
                                                          const std::vector<ObjectId>& ids)
{
    static_cast<void>(page);
    static_cast<void>(pageIndex);
    static_cast<void>(ids);
    return nullptr;
}

} // namespace actcraft::core
