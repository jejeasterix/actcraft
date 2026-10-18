#include <actcraft/core/objectedits.h>

// Talon L0-S : rempli par L3-T4.

namespace actcraft::core {

TransformObjectsCommand::TransformObjectsCommand(std::size_t page, std::vector<ObjectId> ids, Transform delta)
    : m_page(page), m_ids(std::move(ids)), m_delta(delta)
{
    static_cast<void>(m_page);
    static_cast<void>(m_delta);
}

void TransformObjectsCommand::redo(Document&, Changes&) {}

void TransformObjectsCommand::undo(Document&, Changes&) {}

RemoveObjectsCommand::RemoveObjectsCommand(std::size_t page, std::vector<ObjectId> ids)
    : m_page(page), m_ids(std::move(ids))
{
    static_cast<void>(m_page);
}

void RemoveObjectsCommand::redo(Document&, Changes&) {}

void RemoveObjectsCommand::undo(Document&, Changes&) {}

ClearPageCommand::ClearPageCommand(std::size_t page, ClearKind kind) : m_page(page), m_kind(kind)
{
    static_cast<void>(m_page);
    static_cast<void>(m_kind);
}

void ClearPageCommand::redo(Document&, Changes&) {}

void ClearPageCommand::undo(Document&, Changes&) {}

ReplacePageCommand::ReplacePageCommand(std::size_t page, Page saved) : m_page(page), m_other(std::move(saved))
{
    static_cast<void>(m_page);
}

void ReplacePageCommand::redo(Document&, Changes&) {}

void ReplacePageCommand::undo(Document&, Changes&) {}

SetTranslucencyCommand::SetTranslucencyCommand(std::size_t page, std::vector<ObjectId> ids,
                                               std::uint8_t value)
    : m_page(page), m_ids(std::move(ids)), m_value(value)
{
    static_cast<void>(m_page);
    static_cast<void>(m_value);
}

void SetTranslucencyCommand::redo(Document&, Changes&) {}

void SetTranslucencyCommand::undo(Document&, Changes&) {}

int SetTranslucencyCommand::mergeId() const noexcept
{
    return -1;
}

bool SetTranslucencyCommand::mergeWith(const UndoCommand&)
{
    return false;
}

PointF selectionCenter(const Page&, const std::vector<ObjectId>&)
{
    return {};
}

Transform scalingAbout(PointF, double, double) noexcept
{
    return {};
}

Transform rotationAbout(PointF, double) noexcept
{
    return {};
}

} // namespace actcraft::core
