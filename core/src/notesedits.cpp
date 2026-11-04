#include <actcraft/core/document.h>
#include <actcraft/core/notesedits.h>

#include <cstddef>
#include <utility>

namespace actcraft::core {

namespace {

bool validPage(const Document& document, int page) noexcept
{
    return page >= 0 && static_cast<std::size_t>(page) < document.pageCount();
}

} // namespace

SetPageNotesCommand::SetPageNotesCommand(int page, std::string html, Clock::time_point when)
    : m_page(page), m_html(std::move(html)), m_when(when)
{}

void SetPageNotesCommand::redo(Document& document, Changes& changes)
{
    if (!validPage(document, m_page)) {
        return;
    }
    const auto index = static_cast<std::size_t>(m_page);
    Page& page = document.page(index);
    m_old = page.notes;
    if (m_old == m_html) {
        return;
    }
    page.notes = m_html;
    changes.push_back(Change{Change::Kind::PageSettingsChanged, index, 0, RectF{}}); // aucun pixel du cache
}

void SetPageNotesCommand::undo(Document& document, Changes& changes)
{
    if (!validPage(document, m_page)) {
        return;
    }
    const auto index = static_cast<std::size_t>(m_page);
    Page& page = document.page(index);
    if (page.notes == m_old) {
        return;
    }
    page.notes = m_old;
    changes.push_back(Change{Change::Kind::PageSettingsChanged, index, 0, RectF{}});
}

int SetPageNotesCommand::mergeId() const noexcept
{
    return kMergeNotes;
}

// Frappe continue : la commande garde le texte d'avant et prend celui de la suivante, tant que deux frappes
// restent à moins de kMergeWindow l'une de l'autre.
bool SetPageNotesCommand::mergeWith(const UndoCommand& other)
{
    const auto* next = dynamic_cast<const SetPageNotesCommand*>(&other);
    if (next == nullptr || next->m_page != m_page || next->m_when < m_when ||
        next->m_when - m_when >= kMergeWindow) {
        return false;
    }
    m_html = next->m_html;
    m_when = next->m_when;
    return true;
}

} // namespace actcraft::core
