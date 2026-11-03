#include <actcraft/core/printlayout.h>

#include <algorithm>
#include <cmath>

namespace actcraft::core {

namespace {

struct PageSpan
{
    std::size_t first = 0;
    std::size_t last = 0; // inclus
};

// Plage de pages à imprimer, bornée au document ; bornes inversées : remises dans l'ordre.
PageSpan pageSpan(const Document& document, const PrintSettings& settings, std::size_t current)
{
    const std::size_t count = document.pageCount();
    switch (settings.range) {
    case PrintRange::All:
        return {0, count - 1};
    case PrintRange::Current: {
        const std::size_t c = std::min(current, count - 1);
        return {c, c};
    }
    case PrintRange::FromTo:
        break;
    }
    const long long lo = std::min(settings.from, settings.to);
    const long long hi = std::max(settings.from, settings.to);
    const long long maxPage = static_cast<long long>(count);
    const long long first = std::clamp(lo, 1LL, maxPage);
    const long long last = std::clamp(hi, 1LL, maxPage);
    return {static_cast<std::size_t>(first - 1), static_cast<std::size_t>(last - 1)};
}

// Nombre de lignes des notes d'une page ; 0 si elles sont vides.
std::size_t noteLines(const Page& page)
{
    if (page.notes.empty())
        return 0;
    return static_cast<std::size_t>(std::count(page.notes.begin(), page.notes.end(), '\n')) + 1;
}

// Cellule de la grille : la page y tient entière, rapport gardé, centrée.
RectF fitInCell(const Page& page, RectF cell)
{
    if (page.width <= 0 || page.height <= 0)
        return cell;
    const double scale = std::min(cell.width / page.width, cell.height / page.height);
    const double w = page.width * scale;
    const double h = page.height * scale;
    return {cell.x + (cell.width - w) / 2, cell.y + (cell.height - h) / 2, w, h};
}

std::vector<Sheet> notesSheets(const Document& document, PageSpan span, const PrintSettings& settings)
{
    std::vector<Sheet> sheets;
    const auto perSheet = static_cast<std::size_t>(kNoteLinesPerSheet);
    if (settings.noteBreak) {
        for (std::size_t p = span.first; p <= span.last; ++p) {
            if (noteLines(document.page(p)) == 0)
                continue;
            Sheet sheet;
            sheet.notes = true;
            sheet.noteOf.push_back(p);
            sheets.push_back(std::move(sheet));
        }
        return sheets;
    }
    // Notes à la suite : kNoteLinesPerSheet lignes par feuille ; une page y figure sur chaque feuille que
    // ses lignes touchent.
    std::size_t used = 0;
    for (std::size_t p = span.first; p <= span.last; ++p) {
        const std::size_t lines = noteLines(document.page(p));
        if (lines == 0)
            continue;
        const std::size_t firstSheet = used / perSheet;
        const std::size_t lastSheet = (used + lines - 1) / perSheet;
        if (sheets.size() <= lastSheet) {
            const std::size_t old = sheets.size();
            sheets.resize(lastSheet + 1);
            for (std::size_t s = old; s <= lastSheet; ++s) {
                sheets[s].notes = true;
                sheets[s].firstNoteLine = static_cast<int>(s * perSheet);
            }
        }
        for (std::size_t s = firstSheet; s <= lastSheet; ++s)
            sheets[s].noteOf.push_back(p);
        used += lines;
    }
    return sheets;
}

} // namespace

std::vector<Sheet> layoutSheets(const Document& document, const PrintSettings& settings, std::size_t current,
                                SizeF sheetMm)
{
    std::vector<Sheet> sheets;
    const PageSpan span = pageSpan(document, settings, current);

    if (settings.notes != PrintNotes::NotesOnly) {
        const int rows = std::max(1, settings.rows);
        const int columns = std::max(1, settings.columns);
        const double margin =
            std::clamp(settings.marginMm, 0.0, std::min(sheetMm.width, sheetMm.height) / 2 - 1e-6);
        const double usableW = std::max(0.0, sheetMm.width - 2 * margin);
        const double usableH = std::max(0.0, sheetMm.height - 2 * margin);
        const double cellW = usableW / columns;
        const double cellH = usableH / rows;
        const auto perSheet = static_cast<std::size_t>(rows) * static_cast<std::size_t>(columns);
        std::size_t slot = 0;
        for (std::size_t p = span.first; p <= span.last; ++p, ++slot) {
            if (slot % perSheet == 0)
                sheets.emplace_back();
            const std::size_t inSheet = slot % perSheet;
            const double col = static_cast<double>(inSheet % static_cast<std::size_t>(columns));
            const double row = static_cast<double>(inSheet / static_cast<std::size_t>(columns));
            const RectF cell{margin + col * cellW, margin + row * cellH, cellW, cellH};
            sheets.back().cells.push_back({p, fitInCell(document.page(p), cell)});
        }
    }
    if (settings.notes != PrintNotes::None) {
        auto notes = notesSheets(document, span, settings);
        sheets.insert(sheets.end(), std::make_move_iterator(notes.begin()),
                      std::make_move_iterator(notes.end()));
    }
    return sheets;
}

int sheetCount(const Document& document, const PrintSettings& settings, std::size_t current, SizeF sheetMm)
{
    const auto one = layoutSheets(document, settings, current, sheetMm).size();
    return static_cast<int>(one) * std::max(1, settings.copies);
}

} // namespace actcraft::core
