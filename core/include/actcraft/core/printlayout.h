#pragma once
#include <actcraft/core/document.h>
#include <actcraft/core/model.h>

#include <vector>

namespace actcraft::core {

// Taille en millimètres (la feuille) ; absente du modèle jusqu'au lot 5.
struct SizeF
{
    double width = 0;
    double height = 0;
};

enum class PrintRange : std::uint8_t
{
    All,
    Current,
    FromTo,
};
enum class PrintNotes : std::uint8_t
{
    None,
    WithPages,
    NotesOnly,
};
struct PrintSettings
{
    int rows = 1;
    int columns = 1;
    bool frames = false;
    bool pageNumbers = false;
    PrintRange range = PrintRange::All;
    int from = 1; // 1-based, inclus
    int to = 1;
    int copies = 1;
    double marginMm = 5.0; // F-670
    bool gray = false;
    PrintNotes notes = PrintNotes::None;
    bool noteBreak = true;
    bool noteSeparator = false;
};
struct SheetCell
{
    std::size_t page = 0;
    RectF target; // millimètres sur la feuille, rapport de la page gardé (page allongée entière)
};
struct Sheet
{
    bool notes = false;
    std::vector<SheetCell> cells;
    std::vector<std::size_t> noteOf;
    // Notes à la suite : rang, dans la suite des lignes de la plage, de la première ligne de cette feuille
    // (0, kNoteLinesPerSheet, 2 × kNoteLinesPerSheet…). Toujours 0 avec un saut de feuille par page.
    int firstNoteLine = 0;
};
inline constexpr int kNoteLinesPerSheet = 60;

std::vector<Sheet> layoutSheets(const Document& document, const PrintSettings& settings, std::size_t current,
                                SizeF sheetMm);
int sheetCount(const Document& document, const PrintSettings& settings, std::size_t current, SizeF sheetMm);

} // namespace actcraft::core
