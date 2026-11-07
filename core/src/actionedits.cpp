#include <actcraft/core/actionedits.h>
#include <actcraft/core/document.h>

#include <algorithm>

namespace actcraft::core {

namespace {

PageObject* findObject(Page& page, ObjectId id)
{
    const auto it = std::find_if(page.objects.begin(), page.objects.end(),
                                 [id](const PageObject& object) { return object.common.id == id; });
    return it == page.objects.end() ? nullptr : &*it;
}

// Aucun pixel du cache (L0-S3) : la zone vide ne vaut ici que pour l'interface, applyToCache l'ignore.
Change noPixelChange(std::size_t page, ObjectId id)
{
    return Change{Change::Kind::PageSettingsChanged, page, id, RectF{}};
}

} // namespace

bool sameActions(const std::vector<Action>& a, const std::vector<Action>& b) noexcept
{
    return std::equal(a.begin(), a.end(), b.begin(), b.end());
}

SetObjectActionsCommand::SetObjectActionsCommand(std::size_t page, std::vector<ObjectId> ids,
                                                 std::vector<Action> actions)
    : m_page(page)
{
    m_lists.reserve(ids.size());
    for (const ObjectId id : ids) {
        m_lists.emplace_back(id, actions);
    }
}

SetObjectActionsCommand::SetObjectActionsCommand(std::size_t page, std::vector<ObjectActions> lists)
    : m_page(page), m_lists(std::move(lists))
{}

void SetObjectActionsCommand::redo(Document& document, Changes& changes)
{
    m_before.clear();
    if (m_page >= document.pageCount()) {
        return;
    }
    Page& page = document.page(m_page);
    for (const auto& [id, actions] : m_lists) {
        PageObject* object = findObject(page, id);
        if (!object || sameActions(object->common.actions, actions)) {
            continue; // absent, ou déjà cette liste : ni changement ni retour
        }
        m_before.emplace_back(id, object->common.actions);
        object->common.actions = actions;
        changes.push_back(noPixelChange(m_page, id));
    }
}

void SetObjectActionsCommand::undo(Document& document, Changes& changes)
{
    if (m_page >= document.pageCount()) {
        return;
    }
    Page& page = document.page(m_page);
    for (auto it = m_before.rbegin(); it != m_before.rend(); ++it) { // ordre inverse de redo
        const auto& [id, before] = *it;
        if (PageObject* object = findObject(page, id)) {
            object->common.actions = before;
            changes.push_back(noPixelChange(m_page, id));
        }
    }
}

} // namespace actcraft::core
