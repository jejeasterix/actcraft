#include <actcraft/core/pagesettings.h>

#include <utility>

namespace actcraft::core {

namespace {

// Talon : lit les membres que le contrat réserve (évite -Wunused-private-field).
template <typename... T> void touch(const T&...) noexcept {}

} // namespace

// Talon L4-T1 : rempli par L4-T4.
bool gridDrawsUnderObjects(const Grid&) noexcept
{
    return false;
}

bool gridDrawsAboveObjects(const Grid&) noexcept
{
    return false;
}

SetGridCommand::SetGridCommand(std::size_t page, Grid grid, bool merge)
    : m_page(page), m_after(std::move(grid)), m_merge(merge)
{}

void SetGridCommand::redo(Document&, Changes&)
{
    touch(m_page, m_after, m_before, m_merge);
}

void SetGridCommand::undo(Document&, Changes&) {}

SetBackgroundCommand::SetBackgroundCommand(std::size_t page, Background background)
    : m_page(page), m_after(std::move(background))
{}

void SetBackgroundCommand::redo(Document&, Changes&)
{
    touch(m_page, m_after, m_before);
}

void SetBackgroundCommand::undo(Document&, Changes&) {}

double nextExtendedHeight(const Page&) noexcept
{
    return 0;
}

ExtendPageCommand::ExtendPageCommand(std::size_t page) : m_page(page) {}

void ExtendPageCommand::redo(Document&, Changes&)
{
    touch(m_page, m_beforeHeight, m_beforeBase);
}

void ExtendPageCommand::undo(Document&, Changes&) {}

} // namespace actcraft::core
