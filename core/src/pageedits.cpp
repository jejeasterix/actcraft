#include <actcraft/core/pageedits.h>

#include <algorithm>
#include <unordered_map>
#include <utility>
#include <variant>

namespace actcraft::core {

namespace {

using IdTable = std::unordered_map<ObjectId, ObjectId>;

// Indices triés, sans doublon : le contrat des commandes, rendu sûr.
std::vector<std::size_t> normalized(std::vector<std::size_t> pages)
{
    std::sort(pages.begin(), pages.end());
    pages.erase(std::unique(pages.begin(), pages.end()), pages.end());
    return pages;
}

// Vrai si `pages` (triés) désigne au moins une page du document.
bool validPages(std::size_t pageCount, const std::vector<std::size_t>& pages) noexcept
{
    return !pages.empty() && pages.back() < pageCount;
}

// Indice d'arrivée de la première page déplacée : `before` moins les pages déplacées situées avant lui.
std::size_t moveTarget(const std::vector<std::size_t>& pages, std::size_t before) noexcept
{
    const auto below = std::lower_bound(pages.begin(), pages.end(), before);
    return before - static_cast<std::size_t>(below - pages.begin());
}

// Vrai si le déplacement change l'ordre des pages (une page au moins n'arrive pas à son indice).
bool changesOrder(std::size_t pageCount, const std::vector<std::size_t>& pages, std::size_t before) noexcept
{
    if (!validPages(pageCount, pages) || before > pageCount) {
        return false;
    }
    const std::size_t target = moveTarget(pages, before);
    for (std::size_t i = 0; i < pages.size(); ++i) {
        if (pages[i] != target + i) {
            return true;
        }
    }
    return false;
}

void follow(ObjectId& reference, const IdTable& table)
{
    const auto found = table.find(reference);
    if (found != table.end()) {
        reference = found->second;
    }
}

// Vrai si chaque objet a un identifiant (withFreshIds en a tiré un pour chacun).
bool allIdentified(const Page& page) noexcept
{
    return std::none_of(page.objects.begin(), page.objects.end(),
                        [](const PageObject& object) { return object.common.id == 0; });
}

} // namespace

Page withFreshIds(Document& document, Page page)
{
    // Première passe : un identifiant neuf par objet, et la table ancien → neuf de toute la page. 0
    // (« aucun ») n'y entre pas ; un identifiant en double garde sa première entrée. Identifiants épuisés :
    // plus rien n'est tiré, les objets restants gardent 0.
    std::vector<ObjectId> fresh(page.objects.size(), 0);
    IdTable table;
    for (std::size_t i = 0; i < page.objects.size(); ++i) {
        fresh[i] = document.allocateId();
        if (fresh[i] == 0) {
            break;
        }
        if (page.objects[i].common.id != 0) {
            table.emplace(page.objects[i].common.id, fresh[i]);
        }
    }
    // Seconde passe : identifiant propre, puis chaque référence interne ; une référence hors de la page reste
    // telle quelle.
    for (std::size_t i = 0; i < page.objects.size(); ++i) {
        PageObject& object = page.objects[i];
        object.common.id = fresh[i];
        for (Action& action : object.common.actions) {
            follow(action.target, table);
        }
        if (auto* group = std::get_if<Group>(&body(object))) {
            for (ObjectId& member : group->members) {
                follow(member, table);
            }
        } else if (auto* link = std::get_if<Connector>(&body(object))) {
            follow(link->objectA, table);
            follow(link->objectB, table);
        }
    }
    return page;
}

std::vector<std::size_t> positionsAfterMove(std::size_t pageCount, const std::vector<std::size_t>& pages,
                                            std::size_t before)
{
    std::vector<std::size_t> moved = normalized(pages);
    if (!validPages(pageCount, moved) || before > pageCount) {
        return moved; // la commande serait sans effet : les pages restent où elles sont
    }
    const std::size_t target = moveTarget(moved, before);
    for (std::size_t i = 0; i < moved.size(); ++i) {
        moved[i] = target + i;
    }
    return moved;
}

MovePagesCommand::MovePagesCommand(std::vector<std::size_t> pages, std::size_t before)
    : m_pages(normalized(std::move(pages))), m_before(before)
{}

// Seules les pages déplacées sortent et rentrent : la page affichée qui ne bouge pas garde son cache
// (applyToCache suit son indice), et l'annulation remet chaque page à son indice d'origine.
void MovePagesCommand::redo(Document& document, Changes& changes)
{
    if (!changesOrder(document.pageCount(), m_pages, m_before)) {
        return;
    }
    // De la plus haute à la plus basse : chacune sort à son indice d'origine, qui est celui du moment.
    std::vector<Page> taken(m_pages.size());
    for (std::size_t i = m_pages.size(); i-- > 0;) {
        taken[i] = document.takePage(m_pages[i]);
        changes.push_back(Change{Change::Kind::PageRemoved, m_pages[i], 0, RectF{}});
    }
    const std::size_t target = moveTarget(m_pages, m_before);
    for (std::size_t i = 0; i < taken.size(); ++i) {
        document.insertPage(target + i, std::move(taken[i]));
        changes.push_back(Change{Change::Kind::PageInserted, target + i, 0, RectF{}});
    }
}

void MovePagesCommand::undo(Document& document, Changes& changes)
{
    // Même nombre de pages qu'avant le redo : même décision.
    if (!changesOrder(document.pageCount(), m_pages, m_before)) {
        return;
    }
    const std::size_t target = moveTarget(m_pages, m_before);
    std::vector<Page> taken(m_pages.size());
    for (std::size_t i = m_pages.size(); i-- > 0;) {
        taken[i] = document.takePage(target + i);
        changes.push_back(Change{Change::Kind::PageRemoved, target + i, 0, RectF{}});
    }
    // Indices d'origine croissants : les pages restées avant chacune sont déjà en place.
    for (std::size_t i = 0; i < taken.size(); ++i) {
        document.insertPage(m_pages[i], std::move(taken[i]));
        changes.push_back(Change{Change::Kind::PageInserted, m_pages[i], 0, RectF{}});
    }
}

RemovePagesCommand::RemovePagesCommand(std::vector<std::size_t> pages) : m_pages(normalized(std::move(pages)))
{}

void RemovePagesCommand::redo(Document& document, Changes& changes)
{
    // Refus : indice hors du document, ou plus aucune page (un document n'est jamais vide).
    if (!validPages(document.pageCount(), m_pages) || m_pages.size() >= document.pageCount()) {
        return;
    }
    // De la plus haute à la plus basse : chaque PageRemoved porte l'indice du moment (applyToCache).
    m_removed.assign(m_pages.size(), Page{});
    for (std::size_t i = m_pages.size(); i-- > 0;) {
        m_removed[i] = document.takePage(m_pages[i]);
        changes.push_back(Change{Change::Kind::PageRemoved, m_pages[i], 0, RectF{}});
    }
}

void RemovePagesCommand::undo(Document& document, Changes& changes)
{
    // Vide si le redo a été refusé.
    for (std::size_t i = 0; i < m_removed.size(); ++i) {
        document.insertPage(m_pages[i], std::move(m_removed[i]));
        changes.push_back(Change{Change::Kind::PageInserted, m_pages[i], 0, RectF{}});
    }
    m_removed.clear();
}

InsertPagesCommand::InsertPagesCommand(std::size_t index, std::vector<Page> pages)
    : m_index(index), m_pages(std::move(pages))
{}

void InsertPagesCommand::redo(Document& document, Changes& changes)
{
    if (m_pages.empty() || m_index > document.pageCount()) {
        return;
    }
    if (!m_fresh) {
        // Premier redo seulement : un rétablir rend les mêmes identifiants (le cache et la sélection s'y fient).
        std::vector<Page> copies;
        copies.reserve(m_pages.size());
        for (const Page& page : m_pages) {
            copies.push_back(withFreshIds(document, page));
            if (!allIdentified(copies.back())) {
                return; // identifiants épuisés : rien n'est inséré (comme AddObjectCommand)
            }
        }
        m_pages = std::move(copies);
        m_fresh = true;
    }
    // Les pages passent au document ; l'annulation les reprend.
    for (std::size_t i = 0; i < m_pages.size(); ++i) {
        document.insertPage(m_index + i, std::move(m_pages[i]));
        changes.push_back(Change{Change::Kind::PageInserted, m_index + i, 0, RectF{}});
    }
}

void InsertPagesCommand::undo(Document& document, Changes& changes)
{
    if (!m_fresh) {
        return; // le redo n'a rien inséré
    }
    for (std::size_t i = m_pages.size(); i-- > 0;) {
        m_pages[i] = document.takePage(m_index + i);
        changes.push_back(Change{Change::Kind::PageRemoved, m_index + i, 0, RectF{}});
    }
}

} // namespace actcraft::core
