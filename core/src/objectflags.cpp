#include <actcraft/core/objectflags.h>

namespace actcraft::core {

// Talon L5-T1 : rempli par L5-T7.
bool flagOfAll(const Page& page, const std::vector<ObjectId>& ids, ObjectFlag flag)
{
    static_cast<void>(page);
    static_cast<void>(ids);
    static_cast<void>(flag);
    return false;
}

// Talon L5-T1 : rempli par L5-T7.
SetObjectFlagCommand::SetObjectFlagCommand(std::size_t page, std::vector<ObjectId> ids, ObjectFlag flag,
                                           bool value)
    : m_page(page), m_ids(std::move(ids)), m_flag(flag), m_value(value)
{}

// Talon L5-T1 : rempli par L5-T7.
void SetObjectFlagCommand::redo(Document& document, Changes& changes)
{
    static_cast<void>(document);
    static_cast<void>(changes);
    static_cast<void>(m_page);
    static_cast<void>(m_flag);
    static_cast<void>(m_value);
    static_cast<void>(m_before);
}

// Talon L5-T1 : rempli par L5-T7.
void SetObjectFlagCommand::undo(Document& document, Changes& changes)
{
    static_cast<void>(document);
    static_cast<void>(changes);
}

} // namespace actcraft::core
