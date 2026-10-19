#include <actcraft/core/edits.h>
#include <actcraft/core/lift.h>

#include <algorithm>
#include <utility>

namespace actcraft::core {

namespace {

// Identifiants soulevés, triés : le cache ne les a plus depuis liftChanges.
std::vector<ObjectId> liftedIds(const std::vector<LiftedObject>& lifted)
{
    std::vector<ObjectId> ids;
    ids.reserve(lifted.size());
    for (const LiftedObject& l : lifted) {
        ids.push_back(l.object.common.id);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

bool isObjectChange(Change::Kind kind) noexcept
{
    return kind == Change::Kind::ObjectInserted || kind == Change::Kind::ObjectRemoved ||
           kind == Change::Kind::ObjectChanged;
}

} // namespace

std::vector<LiftedObject> takeObjects(Page& page, const std::vector<ObjectId>& ids)
{
    std::vector<std::size_t> ranks;
    for (const ObjectId id : ids) {
        const std::size_t rank = indexOf(page, id);
        if (rank < page.objects.size()) {
            ranks.push_back(rank);
        }
    }
    std::sort(ranks.begin(), ranks.end());
    ranks.erase(std::unique(ranks.begin(), ranks.end()), ranks.end());

    std::vector<LiftedObject> lifted;
    lifted.reserve(ranks.size());
    // Rangs relevés d'abord, retraits du plus haut au plus bas : les rangs plus bas restent valables.
    for (auto it = ranks.rbegin(); it != ranks.rend(); ++it) {
        LiftedObject l;
        l.index = *it;
        l.object = std::move(page.objects[*it]);
        page.objects.erase(page.objects.begin() + static_cast<std::ptrdiff_t>(*it));
        lifted.push_back(std::move(l));
    }
    std::reverse(lifted.begin(), lifted.end()); // ordre de la page
    return lifted;
}

void restoreObjects(Page& page, const std::vector<LiftedObject>& lifted)
{
    // Rangs croissants : chaque objet retrouve le rang qu'il avait dans la page entière.
    for (const LiftedObject& l : lifted) {
        const std::size_t at = std::min(l.index, page.objects.size());
        page.objects.insert(page.objects.begin() + static_cast<std::ptrdiff_t>(at), l.object);
    }
}

Changes liftChanges(std::size_t page, const std::vector<LiftedObject>& lifted)
{
    Changes changes;
    changes.reserve(lifted.size());
    for (const LiftedObject& l : lifted) {
        changes.push_back(
            Change{Change::Kind::ObjectRemoved, page, l.object.common.id, boundsOnPage(l.object)});
    }
    return changes;
}

Changes changesAfterLift(std::size_t page, const std::vector<LiftedObject>& lifted, const Changes& command,
                         const Page& pageAfter)
{
    const std::vector<ObjectId> ids = liftedIds(lifted);
    const auto isLifted = [&ids](ObjectId id) { return std::binary_search(ids.begin(), ids.end(), id); };

    Changes changes;
    // Changements de la commande : ceux d'un objet soulevé n'atteignent pas le cache, qui ne l'a plus (ni
    // invalidation d'un objet absent, ni second retrait d'un trait gommé) ; les autres passent, dans l'ordre.
    for (const Change& c : command) {
        if (c.page == page && isObjectChange(c.kind) && isLifted(c.object)) {
            continue;
        }
        changes.push_back(c);
    }
    // Chaque objet soulevé encore dans la page revient au cache, dans l'ordre de la page.
    for (const PageObject& object : pageAfter.objects) {
        if (isLifted(object.common.id)) {
            changes.push_back(
                Change{Change::Kind::ObjectInserted, page, object.common.id, boundsOnPage(object)});
        }
    }
    return changes;
}

} // namespace actcraft::core
