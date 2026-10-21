#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/textedits.h>

#include <utility>

namespace actcraft::core {

namespace {

// Texte de l'objet `id` sur la page, ou nullptr (objet absent ou d'un autre type).
Text* textOf(Document& document, std::size_t pageIndex, ObjectId id)
{
    Page& page = document.page(pageIndex);
    const std::size_t rank = indexOf(page, id);
    if (rank == page.objects.size())
        return nullptr;
    return std::get_if<Text>(&body(page.objects[rank]));
}

// Remplace le texte par `value`, rend l'ObjectChanged (zone = emprise avant ∪ après).
Change apply(Document& document, std::size_t pageIndex, ObjectId id, const Text& value)
{
    Page& page = document.page(pageIndex);
    PageObject& object = page.objects[indexOf(page, id)];
    const RectF before = boundsOnPage(object);
    std::get<Text>(body(object)) = value;
    return Change{Change::Kind::ObjectChanged, pageIndex, id, unitedRect(before, boundsOnPage(object))};
}

} // namespace

SetTextCommand::SetTextCommand(std::size_t page, ObjectId id, Text text)
    : m_page(page), m_id(id), m_after(std::move(text))
{}

void SetTextCommand::redo(Document& document, Changes& changes)
{
    m_applied = false;
    const Text* text = textOf(document, m_page, m_id);
    if (text == nullptr)
        return;
    m_before = *text;
    m_applied = true;
    changes.push_back(apply(document, m_page, m_id, m_after));
}

void SetTextCommand::undo(Document& document, Changes& changes)
{
    if (!m_applied || textOf(document, m_page, m_id) == nullptr)
        return;
    changes.push_back(apply(document, m_page, m_id, m_before));
}

std::optional<PointF> textBaseline(const Text& text) noexcept
{
    if (text.runs.empty())
        return std::nullopt;
    return text.runs.front().baseline;
}

} // namespace actcraft::core
