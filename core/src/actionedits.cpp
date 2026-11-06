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

SetObjectActionsCommand::SetObjectActionsCommand(std::size_t page, std::vector<ObjectId> ids,
                                                 std::vector<Action> actions)
    : m_page(page), m_ids(std::move(ids)), m_actions(std::move(actions))
{}

void SetObjectActionsCommand::redo(Document& document, Changes& changes)
{
    m_before.clear();
    if (m_page >= document.pageCount()) {
        return;
    }
    Page& page = document.page(m_page);
    for (const ObjectId id : m_ids) {
        PageObject* object = findObject(page, id);
        if (!object) {
            continue;
        }
        m_before.emplace_back(id, object->common.actions);
        object->common.actions = m_actions;
        changes.push_back(noPixelChange(m_page, id));
    }
}

void SetObjectActionsCommand::undo(Document& document, Changes& changes)
{
    if (m_page >= document.pageCount()) {
        return;
    }
    Page& page = document.page(m_page);
    for (auto& [id, before] : m_before) {
        if (PageObject* object = findObject(page, id)) {
            object->common.actions = before;
            changes.push_back(noPixelChange(m_page, id));
        }
    }
}

} // namespace actcraft::core
