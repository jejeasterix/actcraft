#include <actcraft/export/pageexport.h>

namespace actcraft::exporting {

// Talon L5-T1 : rempli par L5-T23.
ImageFormat imageFormatFor(const QUrl& url)
{
    static_cast<void>(url);
    return ImageFormat::Png;
}

// Talon L5-T1 : rempli par L5-T23.
QFuture<ExportResult> exportPageImage(core::Document document, std::size_t page, QUrl url)
{
    static_cast<void>(document);
    static_cast<void>(page);
    static_cast<void>(url);
    return QtFuture::makeReadyValueFuture(ExportResult{});
}

} // namespace actcraft::exporting
