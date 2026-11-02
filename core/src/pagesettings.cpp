#include <actcraft/core/pagesettings.h>

#include <algorithm>
#include <utility>

namespace actcraft::core {

namespace {

// Toute la page (zone vide, contrat de Change) : applyToCache la redessine.
Change pageChanged(std::size_t page) noexcept
{
    return Change{Change::Kind::PageChanged, page, 0, RectF{}};
}

// Vu par l'interface (miniature, surcouche de la grille du dessus), ignoré par applyToCache (I3, L0-S3).
Change pageSettingsChanged(std::size_t page) noexcept
{
    return Change{Change::Kind::PageSettingsChanged, page, 0, RectF{}};
}

bool hasVisibleLevel(const Grid& grid) noexcept
{
    return std::any_of(grid.levels.begin(), grid.levels.end(),
                       [](const GridLevel& level) { return level.visible; });
}

// Un seul Change pour le passage de `from` à `to`. Le cache ne contient que la grille du dessous : il ne
// redessine la page que si elle dessinait ou dessine quelque chose sous les objets. Une grille du dessus est
// en surcouche (L4-T7) : la redessiner dans le cache serait un redessin complet pour rien.
Change gridChange(std::size_t page, const Grid& from, const Grid& to) noexcept
{
    if (gridDrawsUnderObjects(from) || gridDrawsUnderObjects(to)) {
        return pageChanged(page);
    }
    return pageSettingsChanged(page);
}

} // namespace

// Même règle que paintPage (render/src/pagepainter.cpp) : sous les objets si elle n'est pas dessinée
// au-dessus ; paintGrid ne dessine rien d'une grille cachée, et seulement ses niveaux visibles.
bool gridDrawsUnderObjects(const Grid& grid) noexcept
{
    return grid.visible && !grid.drawnAbove && hasVisibleLevel(grid);
}

bool gridDrawsAboveObjects(const Grid& grid) noexcept
{
    return grid.visible && grid.drawnAbove && hasVisibleLevel(grid);
}

SetGridCommand::SetGridCommand(std::size_t page, Grid grid, bool merge)
    : m_page(page), m_after(std::move(grid)), m_merge(merge)
{}

void SetGridCommand::redo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    if (!m_executed) {
        // La grille d'avant, une fois : une commande fusionnée garde celle de la première.
        m_before = page.grid;
        m_executed = true;
    }
    page.grid = m_after;
    changes.push_back(gridChange(m_page, m_before, m_after));
}

void SetGridCommand::undo(Document& document, Changes& changes)
{
    document.page(m_page).grid = m_before;
    changes.push_back(gridChange(m_page, m_after, m_before));
}

int SetGridCommand::mergeId() const noexcept
{
    return kMergeGrid;
}

// Séance du créateur (merge vrai, même page) : la commande garde sa grille d'avant et prend celle de la
// suivante. UndoStack::closeMerge(), à l'ouverture et à la fermeture du créateur, borne la séance (L4-T13).
bool SetGridCommand::mergeWith(const UndoCommand& other)
{
    const auto* next = dynamic_cast<const SetGridCommand*>(&other);
    if (next == nullptr || !next->m_merge || next->m_page != m_page) {
        return false;
    }
    m_after = next->m_after;
    return true;
}

SetBackgroundCommand::SetBackgroundCommand(std::size_t page, Background background)
    : m_page(page), m_after(std::move(background))
{}

void SetBackgroundCommand::redo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    m_before = page.background;
    page.background = m_after;
    changes.push_back(pageChanged(m_page));
}

void SetBackgroundCommand::undo(Document& document, Changes& changes)
{
    document.page(m_page).background = m_before;
    changes.push_back(pageChanged(m_page));
}

// Pas cumulé : 10 % de la hauteur courante à chaque clic (L4-O1 : 768 × 1,1³ = 1022,208, lu 1 022,21 dans
// l'original ; le pas fixe de 10 % de la hauteur d'origine, 998,4, est contredit).
double nextExtendedHeight(const Page& page) noexcept
{
    return page.height * (1.0 + kExtendRatio);
}

ExtendPageCommand::ExtendPageCommand(std::size_t page) : m_page(page) {}

void ExtendPageCommand::redo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    m_beforeHeight = page.height;
    m_beforeBase = page.baseHeight;
    const double next = nextExtendedHeight(page);
    if (page.baseHeight <= 0) {
        page.baseHeight = page.height; // premier clic : la hauteur d'origine
    }
    page.height = next;
    changes.push_back(pageChanged(m_page));
}

void ExtendPageCommand::undo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    page.height = m_beforeHeight;
    page.baseHeight = m_beforeBase;
    changes.push_back(pageChanged(m_page));
}

} // namespace actcraft::core
