#pragma once

#include <actcraft/core/document.h>
#include <actcraft/core/undo.h>

#include <cstddef>
#include <cstdint>

namespace actcraft::core {

inline constexpr double kExtendRatio = 0.10; // F-301 : 10 % par clic (D10)
inline constexpr int kMergeGrid = 4;         // famille de fusion de SetGridCommand (une séance du créateur)

// Vrai si la grille dessine quelque chose sous les objets : visible, non dessinée au-dessus, un niveau
// visible.
bool gridDrawsUnderObjects(const Grid& grid) noexcept;
// Vrai si la grille dessine quelque chose au-dessus des objets (surcouche, L4-T7).
bool gridDrawsAboveObjects(const Grid& grid) noexcept;

// Grille de la page remplacée (F-620 à F-623). Changes : PageChanged (toute la page) si la grille du dessous
// change (avant ou après) ; sinon PageSettingsChanged (ignoré par le cache, L0-S3). `merge` vrai :
// fusion avec la précédente SetGridCommand de la même page (kMergeGrid).
class SetGridCommand final : public UndoCommand
{
public:
    SetGridCommand(std::size_t page, Grid grid, bool merge = false);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    Grid m_after;
    Grid m_before;
    bool m_merge;
};

// Fond de la page remplacé (F-610). Changes : PageChanged (toute la page).
class SetBackgroundCommand final : public UndoCommand
{
public:
    SetBackgroundCommand(std::size_t page, Background background);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    Background m_after;
    Background m_before;
};

// Hauteur après un clic d'extension : height + kExtendRatio × base, base = baseHeight, ou height si 0 (D10).
double nextExtendedHeight(const Page& page) noexcept;

// Un clic d'extension (F-301 a) : baseHeight posé au premier clic, height = nextExtendedHeight. Une étape par
// clic. Changes : PageChanged (toute la page).
class ExtendPageCommand final : public UndoCommand
{
public:
    explicit ExtendPageCommand(std::size_t page);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    double m_beforeHeight = 0;
    double m_beforeBase = 0;
};

} // namespace actcraft::core
