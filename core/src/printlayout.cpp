#include <actcraft/core/printlayout.h>

namespace actcraft::core {

// Talon L5-T1 : rempli par L5-T26.
std::vector<Sheet> layoutSheets(const Document& document, const PrintSettings& settings, std::size_t current,
                                SizeF sheetMm)
{
    static_cast<void>(document);
    static_cast<void>(settings);
    static_cast<void>(current);
    static_cast<void>(sheetMm);
    return {};
}

// Talon L5-T1 : rempli par L5-T26.
int sheetCount(const Document& document, const PrintSettings& settings, std::size_t current, SizeF sheetMm)
{
    static_cast<void>(document);
    static_cast<void>(settings);
    static_cast<void>(current);
    static_cast<void>(sheetMm);
    return 0;
}

} // namespace actcraft::core
