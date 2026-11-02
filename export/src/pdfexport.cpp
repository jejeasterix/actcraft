#include <actcraft/export/pdfexport.h>

namespace actcraft::exporting {

// Talon L5-T1 : rempli par L5-T23.
QFuture<ExportResult> exportPdf(core::Document document, QUrl url, std::vector<std::size_t> pages)
{
    static_cast<void>(document);
    static_cast<void>(url);
    static_cast<void>(pages);
    return QtFuture::makeReadyValueFuture(ExportResult{});
}

// Talon L5-T1 : rempli par L5-T23.
int countPdfPages(const QByteArray& pdf)
{
    static_cast<void>(pdf);
    return 0;
}

} // namespace actcraft::exporting
