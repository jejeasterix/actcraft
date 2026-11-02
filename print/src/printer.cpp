#include <actcraft/print/printer.h>

namespace actcraft::print {

// Talon L5-T1 : rempli par L5-T27.
bool printingAvailable()
{
    return false;
}

// Talon L5-T1 : rempli par L5-T27.
QStringList printerNames()
{
    return {};
}

// Talon L5-T1 : rempli par L5-T27.
QString defaultPrinterName()
{
    return {};
}

// Talon L5-T1 : rempli par L5-T27.
QImage renderSheet(const core::Document& document, const core::Sheet& sheet,
                   const core::PrintSettings& settings, core::SizeF sheetMm, int dpi)
{
    static_cast<void>(document);
    static_cast<void>(sheet);
    static_cast<void>(settings);
    static_cast<void>(sheetMm);
    static_cast<void>(dpi);
    return {};
}

// Talon L5-T1 : rempli par L5-T27.
PrintResult printDocument(const core::Document& document, std::size_t current, const PrintJob& job)
{
    static_cast<void>(document);
    static_cast<void>(current);
    static_cast<void>(job);
    return {};
}

} // namespace actcraft::print
