#include <actcraft/core/containers.h>
#include <actcraft/core/edits.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <variant>

namespace actcraft::core {
namespace {

// « Devant » sur les indices de la page : rang de couche d'abord, puis l'ordre dans la page. Le rang passe
// avant l'indice, même sur une page qui ne serait pas rangée par couche.
bool inFrontAt(const Page& page, std::size_t candidate, std::size_t container) noexcept
{
    if (candidate == container) {
        return false;
    }
    const auto a = static_cast<int>(page.objects[candidate].common.layer);
    const auto b = static_cast<int>(page.objects[container].common.layer);
    return a != b ? a > b : candidate > container;
}

// Indices des objets dessinés pour l'objet d'indice `index` : lui-même, ou pour un groupe ses membres sur la
// page (groupes imbriqués compris, cycle protégé). Un groupe est rangé au sommet de sa couche mais n'est pas
// dessiné à ce rang : seuls ses membres le sont, chacun au sien.
std::vector<std::size_t> drawnAt(const Page& page, std::size_t index)
{
    std::vector<std::size_t> drawn;
    std::vector<std::size_t> seen;
    std::vector<std::size_t> pending{index};
    while (!pending.empty()) {
        const std::size_t i = pending.back();
        pending.pop_back();
        if (std::find(seen.begin(), seen.end(), i) != seen.end()) {
            continue;
        }
        seen.push_back(i);
        const auto* group = std::get_if<Group>(&body(page.objects[i]));
        if (group == nullptr) {
            drawn.push_back(i);
            continue;
        }
        for (const ObjectId member : group->members) {
            const std::size_t m = indexOf(page, member);
            if (m < page.objects.size()) {
                pending.push_back(m);
            }
        }
    }
    return drawn;
}

// Tout ce qui est dessiné pour le candidat est devant tout ce qui est dessiné pour le conteneur.
bool drawnInFront(const Page& page, const std::vector<std::size_t>& candidate,
                  const std::vector<std::size_t>& container)
{
    if (candidate.empty() || container.empty()) {
        return false;
    }
    return std::all_of(candidate.begin(), candidate.end(), [&](std::size_t a) {
        return std::all_of(container.begin(), container.end(),
                           [&](std::size_t b) { return inFrontAt(page, a, b); });
    });
}

bool shareKeyword(const std::vector<std::string>& a, const std::vector<std::string>& b)
{
    return std::any_of(a.begin(), a.end(),
                       [&b](const std::string& k) { return std::find(b.begin(), b.end(), k) != b.end(); });
}

bool contains(const std::vector<ObjectId>& ids, ObjectId id)
{
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

RectF united(const RectF& a, const RectF& b) noexcept
{
    const double left = std::min(a.x, b.x);
    const double top = std::min(a.y, b.y);
    const double right = std::max(a.x + a.width, b.x + b.width);
    const double bottom = std::max(a.y + a.height, b.y + b.height);
    return {left, top, right - left, bottom - top};
}

// Règle FitRule sur deux emprises de page.
bool fitsArea(const ContainerRule& rule, const RectF& box, const RectF& area)
{
    if (rule.fit == FitRule::FullyInside) {
        return area.x >= box.x && area.y >= box.y && area.x + area.width <= box.x + box.width &&
               area.y + area.height <= box.y + box.height;
    }
    const PointF c = box.center();
    const PointF d = area.center();
    const double distance = std::hypot(d.x - c.x, d.y - c.y);
    return distance <= rule.tolerance;
}

// Identifiants déplacés, plus les membres des groupes déplacés (groupes imbriqués compris, cycle protégé) :
// ils bougent avec le reste et ne peuvent pas contenir ce qu'ils accompagnent.
std::vector<ObjectId> movingIds(const Page& page, const std::vector<ObjectId>& moved)
{
    std::vector<ObjectId> ids;
    std::vector<ObjectId> pending = moved;
    while (!pending.empty()) {
        const ObjectId id = pending.back();
        pending.pop_back();
        if (contains(ids, id)) {
            continue;
        }
        ids.push_back(id);
        const std::size_t index = indexOf(page, id);
        if (index >= page.objects.size()) {
            continue;
        }
        if (const auto* group = std::get_if<Group>(&body(page.objects[index]))) {
            pending.insert(pending.end(), group->members.begin(), group->members.end());
        }
    }
    return ids;
}

} // namespace

bool isInFront(const Page& page, ObjectId candidate, ObjectId container)
{
    const std::size_t i = indexOf(page, candidate);
    const std::size_t j = indexOf(page, container);
    if (i >= page.objects.size() || j >= page.objects.size()) {
        return false;
    }
    return drawnInFront(page, drawnAt(page, i), drawnAt(page, j));
}

bool accepts(const PageObject& container, const PageObject& candidate)
{
    const ContainerRule& rule = container.common.container;
    switch (rule.accepts) {
    case Accepts::Nothing:
        return false;
    case Accepts::Anything:
        return true;
    case Accepts::OneObject:
        return rule.object != 0 && candidate.common.id == rule.object;
    case Accepts::Keywords:
        return shareKeyword(rule.keywords, candidate.common.keywords);
    }
    return false;
}

DropResult resolveDrop(const Page& page, const std::vector<ObjectId>& moved)
{
    std::vector<std::size_t> movedAt;
    std::vector<std::size_t> movedDrawn;
    std::optional<RectF> area;
    for (const ObjectId id : moved) {
        const std::size_t index = indexOf(page, id);
        if (index >= page.objects.size()) {
            continue;
        }
        movedAt.push_back(index);
        const std::vector<std::size_t> drawn = drawnAt(page, index);
        movedDrawn.insert(movedDrawn.end(), drawn.begin(), drawn.end());
        const PageObject& object = page.objects[index];
        if (hasExtent(page, object)) {
            const RectF r = boundsOnPage(page, object);
            area = area ? united(*area, r) : r;
        }
    }
    if (movedAt.empty() || !area) {
        return {};
    }
    const std::vector<ObjectId> moving = movingIds(page, moved);
    const PointF dropPoint = area->center();
    std::optional<DropResult> bounce;
    // Du plus haut au plus bas dans la superposition : le premier qui accepte et contient l'emporte.
    for (std::size_t k = page.objects.size(); k-- > 0;) {
        const PageObject& candidateContainer = page.objects[k];
        const ContainerRule& rule = candidateContainer.common.container;
        if (rule.accepts == Accepts::Nothing || !candidateContainer.common.visible ||
            contains(moving, candidateContainer.common.id) || !hasExtent(page, candidateContainer)) {
            continue;
        }
        const bool inFront = drawnInFront(page, movedDrawn, drawnAt(page, k));
        if (!inFront) {
            continue; // un objet derrière le conteneur n'y entre jamais, quelle que soit sa position
        }
        const RectF box = boundsOnPage(page, candidateContainer);
        const bool accepted = std::all_of(movedAt.begin(), movedAt.end(), [&](std::size_t index) {
            return accepts(candidateContainer, page.objects[index]);
        });
        if (accepted && fitsArea(rule, box, *area)) {
            return {DropOutcome::Contained, candidateContainer.common.id, rule.chime};
        }
        if (!bounce && rule.bounceBack && box.contains(dropPoint)) {
            bounce = DropResult{DropOutcome::BounceBack, candidateContainer.common.id, false};
        }
    }
    return bounce ? *bounce : DropResult{};
}

std::vector<ObjectId> containedIn(const Page& page, ObjectId container)
{
    std::vector<ObjectId> inside;
    const std::size_t at = indexOf(page, container);
    if (at >= page.objects.size()) {
        return inside;
    }
    for (std::size_t i = 0; i < page.objects.size(); ++i) {
        if (i == at) {
            continue;
        }
        const ObjectId id = page.objects[i].common.id;
        const DropResult r = resolveDrop(page, {id});
        if (r.outcome == DropOutcome::Contained && r.container == container) {
            inside.push_back(id);
        }
    }
    return inside;
}

} // namespace actcraft::core
