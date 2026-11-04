#include <actcraft/core/arrange.h>
#include <actcraft/core/clipboardedits.h>
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/replace.h>

#include <algorithm>
#include <numeric>
#include <optional>
#include <set>
#include <unordered_map>
#include <utility>
#include <variant>

namespace actcraft::core {

namespace {

// Hauteur d'imbrication de chaque objet de `objects` : 0 hors groupe, 1 pour un groupe sans sous-groupe parmi
// `objects`, 1 + la plus haute de ses sous-groupes sinon. Un cycle est coupé (le groupe en cours de calcul
// compte pour 0).
std::vector<std::size_t> groupHeights(const std::vector<PageObject>& objects)
{
    std::unordered_map<ObjectId, std::size_t> indexById;
    for (std::size_t i = 0; i < objects.size(); ++i) {
        indexById.emplace(objects[i].common.id, i);
    }
    std::vector<std::size_t> height(objects.size(), 0);
    std::vector<bool> visiting(objects.size(), false);
    const auto compute = [&](const auto& self, std::size_t i) -> std::size_t {
        const auto* group = std::get_if<Group>(&objects[i]);
        if (group == nullptr || height[i] != 0 || visiting[i]) {
            return height[i];
        }
        visiting[i] = true;
        std::size_t h = 1;
        for (const ObjectId member : group->members) {
            const auto it = indexById.find(member);
            if (it != indexById.end()) {
                h = std::max(h, self(self, it->second) + 1);
            }
        }
        visiting[i] = false;
        height[i] = h;
        return h;
    };
    for (std::size_t i = 0; i < objects.size(); ++i) {
        compute(compute, i);
    }
    return height;
}

} // namespace

std::vector<PageObject> closureForCopy(const Page& page, const std::vector<ObjectId>& ids)
{
    const std::vector<ObjectId> expanded = expandGroups(page, ids);
    const std::set<ObjectId> wanted(expanded.begin(), expanded.end());
    std::vector<PageObject> out;
    for (const PageObject& object : page.objects) {
        if (wanted.count(object.common.id) == 0) {
            continue;
        }
        PageObject copy = object;
        if (auto* link = std::get_if<Connector>(&copy)) {
            if (wanted.count(link->objectA) == 0) {
                link->objectA = 0;
            }
            if (wanted.count(link->objectB) == 0) {
                link->objectB = 0;
            }
        }
        out.push_back(std::move(copy));
    }
    return out;
}

std::vector<PageObject> preparePaste(std::vector<PageObject> objects, double dx, double dy)
{
    // Ordre : objets simples (ordre reçu), groupes du plus intérieur au plus extérieur, connecteurs en
    // dernier.
    const std::vector<std::size_t> height = groupHeights(objects);
    const auto key = [&](std::size_t i) -> std::pair<int, std::size_t> {
        if (std::holds_alternative<Group>(body(objects[i]))) {
            return {1, height[i]};
        }
        if (std::holds_alternative<Connector>(body(objects[i]))) {
            return {2, 0};
        }
        return {0, 0};
    };
    std::vector<std::size_t> order(objects.size());
    std::iota(order.begin(), order.end(), std::size_t(0));
    std::stable_sort(order.begin(), order.end(),
                     [&](std::size_t a, std::size_t b) { return key(a) < key(b); });

    // Rangs provisoires 1..n, dans cet ordre : ceux de ReplaceObjectsCommand::addedIds()
    // (PasteObjectsCommand).
    std::unordered_map<ObjectId, ObjectId> rankOf;
    for (std::size_t position = 0; position < order.size(); ++position) {
        rankOf.emplace(objects[order[position]].common.id, static_cast<ObjectId>(position + 1));
    }
    const auto rank = [&](ObjectId id) -> ObjectId {
        const auto it = id == 0 ? rankOf.end() : rankOf.find(id);
        return it == rankOf.end() ? 0 : it->second;
    };

    std::vector<PageObject> out;
    out.reserve(objects.size());
    for (const std::size_t i : order) {
        PageObject copy = std::move(objects[i]);
        if (auto* group = std::get_if<Group>(&copy)) {
            std::vector<ObjectId> members;
            for (const ObjectId member : group->members) {
                if (const ObjectId r = rank(member); r != 0) {
                    members.push_back(r); // membre absent des copies : retiré du groupe
                }
            }
            group->members = std::move(members);
        } else if (auto* link = std::get_if<Connector>(&copy)) {
            link->objectA = rank(link->objectA);
            link->objectB = rank(link->objectB);
        }
        copy.common.id = rank(copy.common.id);
        copy.common.name.clear();
        copy.common.locked = false;
        copy.common.transform = compose(copy.common.transform, translation(dx, dy));
        out.push_back(std::move(copy));
    }
    return out;
}

std::vector<PageObject> pasteOnto(std::vector<PageObject> objects, const PageObject& target,
                                  const RectF& targetBounds)
{
    std::optional<RectF> united;
    for (const PageObject& object : objects) {
        if (hasExtent(object)) {
            const RectF r = boundsOnPage(object);
            united = united ? enclosingRect(*united, r) : r;
        }
    }
    const PointF to = targetBounds.center();
    const double dx = united ? to.x - united->center().x : 0;
    const double dy = united ? to.y - united->center().y : 0;
    std::vector<PageObject> out = preparePaste(std::move(objects), dx, dy);
    for (PageObject& object : out) {
        object.common.layer = target.common.layer;
    }
    return out;
}

std::vector<PageObject> preparePasteOnto(std::vector<PageObject> objects, const PageObject& target)
{
    return pasteOnto(std::move(objects), target, boundsOnPage(target));
}

std::vector<PageObject> preparePasteOnto(std::vector<PageObject> objects, const Page& page,
                                         const PageObject& target)
{
    return pasteOnto(std::move(objects), target, boundsOnPage(page, target));
}

PasteObjectsCommand::PasteObjectsCommand(std::size_t page, std::vector<PageObject> prepared)
    : m_inner(std::make_unique<ReplaceObjectsCommand>(page, std::vector<ObjectId>{}, std::move(prepared))),
      m_page(page)
{}

void PasteObjectsCommand::redo(Document& document, Changes& changes)
{
    // Identifiants tirés par ReplaceObjectsCommand au premier redo, gardés ensuite : la réécriture est la
    // même à chaque redo. Les références ne changent aucun pixel : aucun Change ici.
    m_inner->redo(document, changes);
    m_ids = static_cast<const ReplaceObjectsCommand&>(*m_inner).addedIds();

    // Rang r → m_ids[r - 1] : addedIds() ne saute que des identifiants épuisés, toujours en fin (allocateId
    // ne rend plus que 0 une fois l'espace épuisé) ; un rang sans identifiant devient 0.
    const auto real = [this](ObjectId r) -> ObjectId {
        return r >= 1 && r <= m_ids.size() ? m_ids[r - 1] : 0;
    };
    Page& page = document.page(m_page);
    for (const ObjectId id : m_ids) {
        const std::size_t at = indexOf(page, id);
        if (at >= page.objects.size()) {
            continue;
        }
        PageObject& object = page.objects[at];
        if (auto* group = std::get_if<Group>(&object)) {
            std::vector<ObjectId> members;
            for (const ObjectId member : group->members) {
                if (const ObjectId r = real(member); r != 0) {
                    members.push_back(r);
                }
            }
            group->members = std::move(members);
        } else if (auto* link = std::get_if<Connector>(&object)) {
            link->objectA = real(link->objectA);
            link->objectB = real(link->objectB);
        }
    }
}

void PasteObjectsCommand::undo(Document& document, Changes& changes)
{
    m_inner->undo(document, changes);
}

const std::vector<ObjectId>& PasteObjectsCommand::addedIds() const noexcept
{
    return m_ids;
}

std::unique_ptr<UndoCommand> makeCutCommand(const Page& page, std::size_t pageIndex,
                                            const std::vector<ObjectId>& ids)
{
    return std::make_unique<ReplaceObjectsCommand>(pageIndex, expandGroups(page, ids),
                                                   std::vector<PageObject>{});
}

std::unique_ptr<PasteObjectsCommand> makeDuplicateCommand(const Page& page, std::size_t pageIndex,
                                                          const std::vector<ObjectId>& ids)
{
    return std::make_unique<PasteObjectsCommand>(
        pageIndex, preparePaste(closureForCopy(page, ids), kDuplicateOffset, kDuplicateOffset));
}

} // namespace actcraft::core
