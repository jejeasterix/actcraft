#include <actcraft/core/arrange.h>
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/replace.h>

#include <algorithm>
#include <set>
#include <unordered_map>
#include <variant>

namespace actcraft::core {

namespace {

constexpr Layer kLayers[] = {Layer::Background, Layer::Bottom, Layer::Middle, Layer::Top};

const Group* groupAt(const Page& page, ObjectId id)
{
    const std::size_t at = indexOf(page, id);
    return at < page.objects.size() ? std::get_if<Group>(&page.objects[at]) : nullptr;
}

void expandInto(const Page& page, ObjectId id, std::set<ObjectId>& seen, std::vector<ObjectId>& out)
{
    if (!seen.insert(id).second) {
        return; // déjà vu : protège d'un cycle
    }
    if (const Group* group = groupAt(page, id)) {
        for (const ObjectId member : group->members) {
            expandInto(page, member, seen, out);
        }
    }
    out.push_back(id);
}

std::vector<ObjectId> pageOrder(const Page& page)
{
    std::vector<ObjectId> order;
    order.reserve(page.objects.size());
    for (const PageObject& object : page.objects) {
        order.push_back(object.common.id);
    }
    return order;
}

// Pose l'ordre `order` (une permutation des objets de la page) ; sans effet sinon. Changes : pour chaque
// objet dont le rang change, un retrait puis une insertion.
void applyOrder(Document& document, std::size_t pageIndex, const std::vector<ObjectId>& order, Changes& changes)
{
    Page& page = document.page(pageIndex);
    if (order.size() != page.objects.size()) {
        return;
    }
    std::unordered_map<ObjectId, std::size_t> oldRank;
    for (std::size_t i = 0; i < page.objects.size(); ++i) {
        oldRank[page.objects[i].common.id] = i;
    }
    std::vector<PageObject> next;
    next.reserve(order.size());
    std::set<ObjectId> used;
    for (const ObjectId id : order) {
        const auto it = oldRank.find(id);
        if (it == oldRank.end() || !used.insert(id).second) {
            return; // pas une permutation
        }
        next.push_back(page.objects[it->second]);
    }
    for (std::size_t i = 0; i < next.size(); ++i) {
        if (oldRank[next[i].common.id] != i) {
            const RectF area = boundsOnPage(next[i]);
            changes.push_back(Change{Change::Kind::ObjectRemoved, pageIndex, next[i].common.id, area});
            changes.push_back(Change{Change::Kind::ObjectInserted, pageIndex, next[i].common.id, area});
        }
    }
    page.objects = std::move(next);
}

} // namespace

std::vector<ObjectId> expandGroups(const Page& page, const std::vector<ObjectId>& ids)
{
    std::set<ObjectId> seen;
    std::vector<ObjectId> out;
    for (const ObjectId id : ids) {
        expandInto(page, id, seen, out);
    }
    return out;
}

ObjectId topLevelGroupOf(const Page& page, ObjectId id)
{
    ObjectId current = id;
    ObjectId top = 0;
    std::set<ObjectId> seen{id};
    for (;;) {
        ObjectId parent = 0;
        for (const PageObject& object : page.objects) {
            const auto* group = std::get_if<Group>(&object);
            if (group != nullptr &&
                std::find(group->members.begin(), group->members.end(), current) != group->members.end()) {
                parent = object.common.id;
                break;
            }
        }
        if (parent == 0 || !seen.insert(parent).second) {
            return top;
        }
        top = parent;
        current = parent;
    }
}

std::vector<ObjectId> arrangedOrder(const Page& page, const std::vector<ObjectId>& selected, ArrangeMove move)
{
    const std::vector<ObjectId> expanded = expandGroups(page, selected);
    const std::set<ObjectId> chosen(expanded.begin(), expanded.end());
    std::vector<ObjectId> result;
    result.reserve(page.objects.size());
    for (const Layer layer : kLayers) {
        std::vector<ObjectId> run;
        for (const PageObject& object : page.objects) {
            if (object.common.layer == layer) {
                run.push_back(object.common.id);
            }
        }
        const auto isChosen = [&chosen](ObjectId id) { return chosen.count(id) != 0; };
        switch (move) {
        case ArrangeMove::Forward:
            for (std::size_t i = run.size(); i-- > 1;) {
                if (isChosen(run[i - 1]) && !isChosen(run[i])) {
                    std::swap(run[i - 1], run[i]);
                }
            }
            break;
        case ArrangeMove::Backward:
            for (std::size_t i = 1; i < run.size(); ++i) {
                if (isChosen(run[i]) && !isChosen(run[i - 1])) {
                    std::swap(run[i], run[i - 1]);
                }
            }
            break;
        case ArrangeMove::Front:
            std::stable_partition(run.begin(), run.end(), [&](ObjectId id) { return !isChosen(id); });
            break;
        case ArrangeMove::Back:
            std::stable_partition(run.begin(), run.end(), isChosen);
            break;
        }
        result.insert(result.end(), run.begin(), run.end());
    }
    return result;
}

ReorderObjectsCommand::ReorderObjectsCommand(std::size_t page, std::vector<ObjectId> newOrder)
    : m_page(page), m_newOrder(std::move(newOrder))
{}

void ReorderObjectsCommand::redo(Document& document, Changes& changes)
{
    if (m_oldOrder.empty()) {
        m_oldOrder = pageOrder(document.page(m_page));
    }
    applyOrder(document, m_page, m_newOrder, changes);
}

void ReorderObjectsCommand::undo(Document& document, Changes& changes)
{
    applyOrder(document, m_page, m_oldOrder, changes);
}

SetLayerCommand::SetLayerCommand(std::size_t page, std::vector<ObjectId> ids, Layer layer)
    : m_page(page), m_ids(std::move(ids)), m_layer(layer)
{}

void SetLayerCommand::redo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    m_before.clear();
    m_orderBefore = pageOrder(page);
    const std::set<ObjectId> wanted(m_ids.begin(), m_ids.end());
    std::vector<PageObject> moved;
    std::vector<PageObject> kept;
    for (PageObject& object : page.objects) {
        if (wanted.count(object.common.id) != 0 && object.common.layer != m_layer) {
            m_before.emplace_back(object.common.id, object.common.layer);
            object.common.layer = m_layer;
            moved.push_back(std::move(object));
        } else {
            kept.push_back(std::move(object));
        }
    }
    page.objects = std::move(kept);
    std::vector<ObjectId> before = m_orderBefore;
    for (PageObject& object : moved) {
        insertAtTopOfLayer(page, std::move(object));
    }
    // Changes : comparaison de l'ordre d'avant et d'après, comme un réordonnancement.
    const std::vector<ObjectId> after = pageOrder(page);
    std::unordered_map<ObjectId, std::size_t> oldRank;
    for (std::size_t i = 0; i < before.size(); ++i) {
        oldRank[before[i]] = i;
    }
    for (std::size_t i = 0; i < after.size(); ++i) {
        if (oldRank[after[i]] != i) {
            const RectF area = boundsOnPage(page.objects[i]);
            changes.push_back(Change{Change::Kind::ObjectRemoved, m_page, after[i], area});
            changes.push_back(Change{Change::Kind::ObjectInserted, m_page, after[i], area});
        }
    }
}

void SetLayerCommand::undo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    for (const auto& [id, layer] : m_before) {
        const std::size_t at = indexOf(page, id);
        if (at < page.objects.size()) {
            page.objects[at].common.layer = layer;
        }
    }
    applyOrder(document, m_page, m_orderBefore, changes);
}

std::unique_ptr<UndoCommand> makeGroupCommand(const Page& page, std::size_t pageIndex,
                                              const std::vector<ObjectId>& ids)
{
    const std::set<ObjectId> wanted(ids.begin(), ids.end());
    Group group;
    Layer lowest = Layer::Top;
    for (const PageObject& object : page.objects) {
        if (wanted.count(object.common.id) != 0) {
            group.members.push_back(object.common.id);
            lowest = std::min(lowest, object.common.layer);
        }
    }
    if (group.members.size() < 2) {
        return nullptr;
    }
    PageObject object = group;
    object.common.layer = lowest;
    return std::make_unique<ReplaceObjectsCommand>(pageIndex, std::vector<ObjectId>{},
                                                   std::vector<PageObject>{std::move(object)});
}

std::unique_ptr<UndoCommand> makeUngroupCommand(const Page& page, std::size_t pageIndex,
                                                const std::vector<ObjectId>& groups)
{
    std::vector<ObjectId> removed;
    for (const ObjectId id : groups) {
        if (groupAt(page, id) != nullptr && std::find(removed.begin(), removed.end(), id) == removed.end()) {
            removed.push_back(id);
        }
    }
    if (removed.empty()) {
        return nullptr;
    }
    return std::make_unique<ReplaceObjectsCommand>(pageIndex, std::move(removed), std::vector<PageObject>{});
}

} // namespace actcraft::core
