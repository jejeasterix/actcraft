#include <actcraft/export/pdfimport.h>

namespace actcraft::exporting {

// Talon L5-T1 : rempli par L5-T24.
bool pdfImportAvailable()
{
    return false;
}

// Talon L5-T1 : rempli par L5-T24.
QFuture<ImportResult> importPdf(QUrl url, int dpi)
{
    static_cast<void>(url);
    static_cast<void>(dpi);
    return QtFuture::makeReadyValueFuture(ImportResult{});
}

} // namespace actcraft::exporting
