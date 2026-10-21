#include <actcraft/core/pageedits.h>

#include <utility>

namespace actcraft::core {

namespace {

// Talon : lit les membres que le contrat réserve (évite -Wunused-private-field).
template <typename... T> void touch(const T&...) noexcept {}

} // namespace

// Talon L4-T1 : rempli par L4-T15.
Page withFreshIds(Document&, Page page)
{
    return page;
}

std::vector<std::size_t> positionsAfterMove(std::size_t, const std::vector<std::size_t>&, std::size_t)
{
    return {};
}

MovePagesCommand::MovePagesCommand(std::vector<std::size_t> pages, std::size_t before)
    : m_pages(std::move(pages)), m_before(before)
{}

void MovePagesCommand::redo(Document&, Changes&)
{
    touch(m_pages, m_before);
}

void MovePagesCommand::undo(Document&, Changes&) {}

RemovePagesCommand::RemovePagesCommand(std::vector<std::size_t> pages) : m_pages(std::move(pages)) {}

void RemovePagesCommand::redo(Document&, Changes&)
{
    touch(m_pages, m_removed);
}

void RemovePagesCommand::undo(Document&, Changes&) {}

InsertPagesCommand::InsertPagesCommand(std::size_t index, std::vector<Page> pages)
    : m_index(index), m_pages(std::move(pages))
{}

void InsertPagesCommand::redo(Document&, Changes&)
{
    touch(m_index, m_pages, m_fresh);
}

void InsertPagesCommand::undo(Document&, Changes&) {}

} // namespace actcraft::core
