#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/objectflags.h>

namespace actcraft::core {

namespace {

// Valeur de l'indicateur « au sens de la bascule » : Hidden est l'inverse de visible.
bool read(const PageObject& object, ObjectFlag flag) noexcept
{
    switch (flag) {
    case ObjectFlag::Locked:
        return object.common.locked;
    case ObjectFlag::Hidden:
        return !object.common.visible;
    case ObjectFlag::DragCopy:
        return object.common.dragCopy;
    }
    return false;
}

void write(PageObject& object, ObjectFlag flag, bool value) noexcept
{
    switch (flag) {
    case ObjectFlag::Locked:
        object.common.locked = value;
        break;
    case ObjectFlag::Hidden:
        object.common.visible = !value;
        break;
    case ObjectFlag::DragCopy:
        object.common.dragCopy = value;
        break;
    }
}

// Seul Hidden change des pixels : verrouiller ou activer la copie au glissement ne redessine rien.
bool changesPixels(ObjectFlag flag) noexcept
{
    return flag == ObjectFlag::Hidden;
}

} // namespace

bool flagOfAll(const Page& page, const std::vector<ObjectId>& ids, ObjectFlag flag)
{
    if (ids.empty()) {
        return false;
    }
    for (const ObjectId id : ids) {
        const std::size_t index = indexOf(page, id);
        if (index >= page.objects.size() || !read(page.objects[index], flag)) {
            return false;
        }
    }
    return true;
}

SetObjectFlagCommand::SetObjectFlagCommand(std::size_t page, std::vector<ObjectId> ids, ObjectFlag flag,
                                           bool value)
    : m_page(page), m_ids(std::move(ids)), m_flag(flag), m_value(value)
{}

void SetObjectFlagCommand::redo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    const bool first = m_before.empty();
    if (first) {
        m_before.reserve(m_ids.size());
    }
    for (std::size_t i = 0; i < m_ids.size(); ++i) {
        const std::size_t index = indexOf(page, m_ids[i]);
        if (index >= page.objects.size()) {
            if (first) {
                m_before.push_back(m_value); // absent : rien à rétablir
            }
            continue;
        }
        PageObject& object = page.objects[index];
        if (first) {
            m_before.push_back(read(object, m_flag));
        }
        write(object, m_flag, m_value);
        if (changesPixels(m_flag)) {
            changes.push_back(
                Change{Change::Kind::ObjectChanged, m_page, m_ids[i], boundsOnPage(page, object)});
        }
    }
}

void SetObjectFlagCommand::undo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    for (std::size_t i = 0; i < m_ids.size() && i < m_before.size(); ++i) {
        const std::size_t index = indexOf(page, m_ids[i]);
        if (index >= page.objects.size()) {
            continue;
        }
        PageObject& object = page.objects[index];
        write(object, m_flag, m_before[i]);
        if (changesPixels(m_flag)) {
            changes.push_back(
                Change{Change::Kind::ObjectChanged, m_page, m_ids[i], boundsOnPage(page, object)});
        }
    }
}

} // namespace actcraft::core
