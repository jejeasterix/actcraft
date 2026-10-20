#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/objectedits.h>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <utility>

namespace actcraft::core {

namespace {

// Rangs (dans page.objects) des objets `ids` présents, croissants, chacun une fois.
std::vector<std::size_t> presentRanks(const Page& page, const std::vector<ObjectId>& ids)
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
    return ranks;
}

// Effacer ▸ (D8) : un objet verrouillé n'est jamais retiré ; Grille et Fond ne retirent aucun objet.
bool clearedBy(ClearKind kind, const PageObject& object) noexcept
{
    if (object.common.locked) {
        return false;
    }
    switch (kind) {
    case ClearKind::Annotations:
        return isAnnotation(object);
    case ClearKind::Objects:
        return !isAnnotation(object); // toutes couches, celle du fond comprise
    case ClearKind::Page:
        return true;
    case ClearKind::Grid:
    case ClearKind::Background:
        return false;
    }
    return false;
}

Change pageChanged(std::size_t page)
{
    return Change{Change::Kind::PageChanged, page, 0, RectF{}};
}

} // namespace

TransformObjectsCommand::TransformObjectsCommand(std::size_t page, std::vector<ObjectId> ids, Transform delta)
    : m_page(page), m_ids(std::move(ids)), m_delta(delta)
{}

void TransformObjectsCommand::redo(Document& document, Changes& changes)
{
    m_before.clear();
    if (m_delta.isIdentity()) {
        return; // rien ne bouge : aucun changement
    }
    Page& page = document.page(m_page);
    for (const std::size_t rank : presentRanks(page, m_ids)) {
        PageObject& object = page.objects[rank];
        const RectF before = boundsOnPage(object);
        m_before.emplace_back(object.common.id, object.common.transform);
        object.common.transform = compose(object.common.transform, m_delta);
        changes.push_back(Change{Change::Kind::ObjectChanged, m_page, object.common.id,
                                 unitedRect(before, boundsOnPage(object))});
    }
}

void TransformObjectsCommand::undo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    for (auto it = m_before.rbegin(); it != m_before.rend(); ++it) {
        const std::size_t rank = indexOf(page, it->first);
        if (rank == page.objects.size()) {
            continue;
        }
        PageObject& object = page.objects[rank];
        const RectF before = boundsOnPage(object);
        object.common.transform = it->second;
        changes.push_back(
            Change{Change::Kind::ObjectChanged, m_page, it->first, unitedRect(before, boundsOnPage(object))});
    }
}

RemoveObjectsCommand::RemoveObjectsCommand(std::size_t page, std::vector<ObjectId> ids)
    : m_page(page), m_ids(std::move(ids))
{}

void RemoveObjectsCommand::redo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    const std::vector<std::size_t> ranks = presentRanks(page, m_ids);
    m_removed.clear();
    // Du plus haut au plus bas : les rangs plus bas restent valables.
    for (auto it = ranks.rbegin(); it != ranks.rend(); ++it) {
        const std::size_t rank = *it;
        PageObject object = std::move(page.objects[rank]);
        page.objects.erase(page.objects.begin() + static_cast<std::ptrdiff_t>(rank));
        changes.push_back(
            Change{Change::Kind::ObjectRemoved, m_page, object.common.id, boundsOnPage(object)});
        m_removed.emplace_back(rank, std::move(object));
    }
    std::reverse(m_removed.begin(), m_removed.end()); // rangs croissants
}

void RemoveObjectsCommand::undo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    for (const auto& [rank, object] : m_removed) {
        const std::size_t at = std::min(rank, page.objects.size());
        page.objects.insert(page.objects.begin() + static_cast<std::ptrdiff_t>(at), object);
        changes.push_back(
            Change{Change::Kind::ObjectInserted, m_page, object.common.id, boundsOnPage(object)});
    }
}

ClearPageCommand::ClearPageCommand(std::size_t page, ClearKind kind) : m_page(page), m_kind(kind) {}

void ClearPageCommand::redo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    m_before = page; // page entière d'avant
    const ClearKind kind = m_kind;
    std::erase_if(page.objects, [kind](const PageObject& object) { return clearedBy(kind, object); });
    if (kind == ClearKind::Grid) {
        page.grid = Grid{};
    } else if (kind == ClearKind::Background) {
        page.background = Background{};
    }
    if (fingerprint(page) != fingerprint(m_before)) {
        changes.push_back(pageChanged(m_page));
    }
}

void ClearPageCommand::undo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    if (fingerprint(page) == fingerprint(m_before)) {
        return; // l'effacement n'avait rien changé
    }
    page = m_before;
    changes.push_back(pageChanged(m_page));
}

ReplacePageCommand::ReplacePageCommand(std::size_t page, Page saved) : m_page(page), m_other(std::move(saved))
{}

// redo et undo échangent la page et l'autre état (D9 : une étape, propre à la page).
void ReplacePageCommand::redo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    if (fingerprint(page) == fingerprint(m_other)) {
        return; // déjà la page enregistrée : rien ne change
    }
    std::swap(page, m_other);
    ObjectId largest = 0;
    for (const PageObject& object : page.objects) {
        largest = std::max(largest, object.common.id);
    }
    document.reserveIdsBelow(largest == std::numeric_limits<ObjectId>::max() ? largest : largest + 1);
    changes.push_back(pageChanged(m_page));
}

void ReplacePageCommand::undo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    if (fingerprint(page) == fingerprint(m_other)) {
        return;
    }
    std::swap(page, m_other);
    changes.push_back(pageChanged(m_page));
}

SetTranslucencyCommand::SetTranslucencyCommand(std::size_t page, std::vector<ObjectId> ids,
                                               std::uint8_t value)
    : m_page(page), m_ids(std::move(ids)), m_value(value)
{}

void SetTranslucencyCommand::redo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    m_before.clear();
    for (const std::size_t rank : presentRanks(page, m_ids)) {
        PageObject& object = page.objects[rank];
        m_before.emplace_back(object.common.id, object.common.translucency);
        if (object.common.translucency == m_value) {
            continue;
        }
        object.common.translucency = m_value;
        changes.push_back(
            Change{Change::Kind::ObjectChanged, m_page, object.common.id, boundsOnPage(object)});
    }
}

void SetTranslucencyCommand::undo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    for (const auto& [id, value] : m_before) {
        const std::size_t rank = indexOf(page, id);
        if (rank == page.objects.size() || page.objects[rank].common.translucency == value) {
            continue;
        }
        PageObject& object = page.objects[rank];
        object.common.translucency = value;
        changes.push_back(Change{Change::Kind::ObjectChanged, m_page, id, boundsOnPage(object)});
    }
}

int SetTranslucencyCommand::mergeId() const noexcept
{
    return kMergeTranslucency;
}

// Réglette glissée : la commande garde son état d'avant et prend la valeur de la suivante.
bool SetTranslucencyCommand::mergeWith(const UndoCommand& other)
{
    const auto* next = dynamic_cast<const SetTranslucencyCommand*>(&other);
    if (next == nullptr || next->m_page != m_page || next->m_ids != m_ids) {
        return false;
    }
    m_value = next->m_value;
    return true;
}

PointF selectionCenter(const Page& page, const std::vector<ObjectId>& ids)
{
    const std::vector<std::size_t> ranks = presentRanks(page, ids);
    if (ranks.empty()) {
        return {};
    }
    RectF united;
    for (const std::size_t rank : ranks) {
        united = unitedRect(united, boundsOnPage(page.objects[rank]));
    }
    return united.center();
}

Transform scalingAbout(PointF center, double sx, double sy) noexcept
{
    return compose(compose(translation(-center.x, -center.y), scaling(sx, sy)),
                   translation(center.x, center.y));
}

double lengthScale(const Transform& t) noexcept
{
    return std::sqrt(std::abs(t.m11 * t.m22 - t.m12 * t.m21));
}

Transform rotationAbout(PointF center, double degrees) noexcept
{
    return compose(compose(translation(-center.x, -center.y), rotation(degrees)),
                   translation(center.x, center.y));
}

} // namespace actcraft::core
