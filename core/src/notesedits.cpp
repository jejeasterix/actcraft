#include <actcraft/core/notesedits.h>

#include <actcraft/core/document.h>

#include <cstddef>
#include <utility>

namespace actcraft::core {

SetPageNotesCommand::SetPageNotesCommand(int page, std::string html, Clock::time_point when)
    : m_page(page), m_html(std::move(html)), m_when(when)
{}

void SetPageNotesCommand::redo(Document&, Changes&) {}
void SetPageNotesCommand::undo(Document&, Changes&) {}
int SetPageNotesCommand::mergeId() const noexcept
{
    return -1;
}
bool SetPageNotesCommand::mergeWith(const UndoCommand&)
{
    return false;
}

} // namespace actcraft::core
