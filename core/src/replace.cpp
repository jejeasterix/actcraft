#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/replace.h>

#include <algorithm>
#include <cassert>
#include <iterator>
#include <utility>

namespace actcraft::core {

namespace {

// Deux réglages de présentation égaux champ à champ (liaisons structurées : un champ ajouté casse la
// compilation au lieu d'échapper à la comparaison).
bool samePresentation(const PagePresentation& a, const PagePresentation& b) noexcept
{
    const auto& [aTools, aCurtain, aDistance, aSpotlight, aFrame, aHasOrigin, aOrigin, aTransition, aKeys] =
        a;
    const auto& [bTools, bCurtain, bDistance, bSpotlight, bFrame, bHasOrigin, bOrigin, bTransition, bKeys] =
        b;
    return aTools == bTools && aCurtain == bCurtain && aDistance == bDistance && aSpotlight == bSpotlight &&
           aFrame.x == bFrame.x && aFrame.y == bFrame.y && aFrame.width == bFrame.width &&
           aFrame.height == bFrame.height && aHasOrigin == bHasOrigin && aOrigin.x == bOrigin.x &&
           aOrigin.y == bOrigin.y && aTransition == bTransition && aKeys == bKeys;
}

} // namespace

ReplaceObjectsCommand::ReplaceObjectsCommand(std::size_t page, std::vector<ObjectReplacement> replacements)
    : m_page(page), m_groups(std::move(replacements))
{}

ReplaceObjectsCommand::ReplaceObjectsCommand(std::size_t page, std::vector<ObjectId> removed,
                                             std::vector<PageObject> added)
    : m_page(page)
{
    m_groups.push_back(ObjectReplacement{std::move(removed), std::move(added)});
}

void ReplaceObjectsCommand::redo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    const bool first = m_applied.empty();
    if (first) {
        m_applied.resize(m_groups.size());
    }
    for (std::size_t g = 0; g < m_groups.size(); ++g) {
        const ObjectReplacement& group = m_groups[g];
        Applied& applied = m_applied[g];

        // Rangs des retirés présents, avant le retrait du groupe (un identifiant cité deux fois compte une
        // fois).
        std::vector<std::size_t> ranks;
        for (const ObjectId id : group.removed) {
            const std::size_t rank = indexOf(page, id);
            if (rank < page.objects.size()) {
                ranks.push_back(rank);
            }
        }
        std::sort(ranks.begin(), ranks.end());
        ranks.erase(std::unique(ranks.begin(), ranks.end()), ranks.end());

        // Ancrage des ajouts : r = rang du plus haut retiré, k = retirés présents sous lui ; à partir de r -
        // k.
        const bool anchored = !ranks.empty();
        const std::size_t r = anchored ? ranks.back() : 0;
        const std::size_t k = anchored ? ranks.size() - 1 : 0;
        const Layer anchorLayer = anchored ? page.objects[r].common.layer : Layer::Background;

        // Retraits, du plus haut au plus bas (les rangs plus bas restent valables) ; l'annulation les remet
        // par rangs croissants, dans l'ordre inverse de ces Changes.
        Changes removedChanges;
        applied.removed.clear();
        for (auto it = ranks.rbegin(); it != ranks.rend(); ++it) {
            const std::size_t rank = *it;
            PageObject object = std::move(page.objects[rank]);
            page.objects.erase(page.objects.begin() + static_cast<std::ptrdiff_t>(rank));
            removedChanges.push_back(
                Change{Change::Kind::ObjectRemoved, m_page, object.common.id, boundsOnPage(object)});
            applied.removed.emplace_back(rank, std::move(object));
        }
        std::reverse(applied.removed.begin(), applied.removed.end()); // rangs croissants

        // Ajouts : identifiants tirés au premier redo, puis gardés (celui que porte l'ajout est ignoré).
        if (first) {
            applied.added.clear();
            for (std::size_t i = 0; i < group.added.size(); ++i) {
                applied.added.push_back(document.allocateId());
            }
        }
        Changes insertedChanges;
        std::size_t next = r - k;
        for (std::size_t i = 0; i < group.added.size(); ++i) {
            const ObjectId id = applied.added[i];
            if (id == 0) {
                continue; // identifiants épuisés : jamais d'objet d'identifiant 0
            }
            PageObject object = group.added[i];
            object.common.id = id;
            const RectF area = boundsOnPage(object);
            if (anchored && object.common.layer == anchorLayer) {
                page.objects.insert(page.objects.begin() + static_cast<std::ptrdiff_t>(next),
                                    std::move(object));
                ++next;
            } else {
                const Layer layer = object.common.layer;
                const std::size_t at = insertAtTopOfLayer(page, std::move(object));
                if (anchored && (at < next || (at == next && layer < anchorLayer))) {
                    ++next; // inséré sous l'ancrage : la suite du groupe glisse d'un rang
                }
            }
            insertedChanges.push_back(Change{Change::Kind::ObjectInserted, m_page, id, area});
        }

        // Contrat de Change (replace.h) : les retraits du groupe, puis ses ajouts.
        changes.insert(changes.end(), removedChanges.begin(), removedChanges.end());
        changes.insert(changes.end(), insertedChanges.begin(), insertedChanges.end());
    }
    if (first) {
        for (const Applied& applied : m_applied) {
            std::copy_if(applied.added.begin(), applied.added.end(), std::back_inserter(m_addedIds),
                         [](ObjectId id) { return id != 0; });
        }
    }
}

void ReplaceObjectsCommand::undo(Document& document, Changes& changes)
{
    if (m_applied.empty()) {
        return; // jamais exécutée
    }
    Page& page = document.page(m_page);
    for (std::size_t g = m_groups.size(); g-- > 0;) {
        const Applied& applied = m_applied[g];
        for (auto it = applied.added.rbegin(); it != applied.added.rend(); ++it) {
            const ObjectId id = *it;
            if (id == 0) {
                continue;
            }
            const std::size_t index = indexOf(page, id);
            assert(index < page.objects.size());
            if (index == page.objects.size()) {
                continue;
            }
            const RectF area = boundsOnPage(page.objects[index]);
            page.objects.erase(page.objects.begin() + static_cast<std::ptrdiff_t>(index));
            changes.push_back(Change{Change::Kind::ObjectRemoved, m_page, id, area});
        }
        for (const auto& [rank, object] : applied.removed) {
            page.objects.insert(page.objects.begin() + static_cast<std::ptrdiff_t>(rank), object);
            changes.push_back(
                Change{Change::Kind::ObjectInserted, m_page, object.common.id, boundsOnPage(object)});
        }
    }
}

const std::vector<ObjectId>& ReplaceObjectsCommand::addedIds() const noexcept
{
    return m_addedIds;
}

SetPagePresentationCommand::SetPagePresentationCommand(std::size_t page, PagePresentation value)
    : m_page(page), m_value(std::move(value))
{}

void SetPagePresentationCommand::redo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    m_previous = page.presentation;
    m_value.unknownKeys = m_previous.unknownKeys; // sous-clés d'une version plus récente : gardées (L2-C8)
    if (samePresentation(m_previous, m_value)) {
        return; // déjà la valeur de la page : rien ne change
    }
    page.presentation = m_value;
    // Ni ObjectChanged ni PageChanged : applyToCache redessinerait la page (I3).
    changes.push_back(Change{Change::Kind::PageSettingsChanged, m_page, 0, RectF{}});
}

void SetPagePresentationCommand::undo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    if (samePresentation(page.presentation, m_previous)) {
        return;
    }
    page.presentation = m_previous;
    changes.push_back(Change{Change::Kind::PageSettingsChanged, m_page, 0, RectF{}});
}

} // namespace actcraft::core
