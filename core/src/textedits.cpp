#include <actcraft/core/textedits.h>

#include <utility>

namespace actcraft::core {

namespace {

// Talon : lit les membres que le contrat réserve (évite -Wunused-private-field).
template <typename... T> void touch(const T&...) noexcept {}

} // namespace

// Talon L4-T1 : rempli par L4-T16.
SetTextCommand::SetTextCommand(std::size_t page, ObjectId id, Text text)
    : m_page(page), m_id(id), m_after(std::move(text))
{}

void SetTextCommand::redo(Document&, Changes&)
{
    touch(m_page, m_id, m_after, m_before);
}

void SetTextCommand::undo(Document&, Changes&) {}

std::optional<PointF> textBaseline(const Text&) noexcept
{
    return std::nullopt;
}

} // namespace actcraft::core
