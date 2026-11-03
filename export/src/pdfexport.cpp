#include "asyncrun.h"

#include <actcraft/export/pdfexport.h>
#include <actcraft/formats/urlio.h>
#include <actcraft/render/pagepainter.h>

#include <QBuffer>
#include <QMarginsF>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QRegularExpression>
#include <QString>

#include <functional>

namespace actcraft::exporting {

namespace detail {
void noteRenderThread(); // pageexport.cpp
} // namespace detail

namespace {

constexpr double kPointsPerPixel = 72.0 / 96.0;

QPageSize pageSizeOf(const core::Page& page)
{
    return QPageSize(QSizeF(page.width, page.height) * kPointsPerPixel, QPageSize::Point);
}

ExportResult writePdf(const core::Document& document, const QUrl& url, std::vector<std::size_t> pages)
{
    if (pages.empty()) {
        for (std::size_t i = 0; i < document.pageCount(); ++i) {
            pages.push_back(i);
        }
    }
    for (const std::size_t index : pages) {
        if (index >= document.pageCount()) {
            return detail::failure<ExportResult>(QStringLiteral("Cette page n'existe pas."));
        }
    }
    detail::noteRenderThread();
    const render::DecodedImages images = render::decodeImages(document);
    const render::ImageLookup lookup = images.lookup();

    QByteArray bytes;
    {
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        QPdfWriter writer(&buffer);
        writer.setResolution(96);
        writer.setCreator(QStringLiteral("ActCraft"));
        writer.setPageMargins(QMarginsF(0, 0, 0, 0), QPageLayout::Point);
        writer.setPageSize(pageSizeOf(document.page(pages.front())));
        QPainter painter;
        if (!painter.begin(&writer)) {
            return detail::failure<ExportResult>(QStringLiteral("Le PDF n'a pas pu être préparé."));
        }
        for (std::size_t n = 0; n < pages.size(); ++n) {
            const core::Page& page = document.page(pages[n]);
            if (n > 0) {
                writer.setPageSize(pageSizeOf(page));
                writer.setPageMargins(QMarginsF(0, 0, 0, 0), QPageLayout::Point);
                writer.newPage();
            }
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            painter.fillRect(QRectF(0, 0, page.width, page.height), Qt::white);
            render::paintPage(painter, page, lookup);
        }
        painter.end();
    }
    QString error;
    if (!formats::writeUrl(url, bytes, &error)) {
        return detail::failure<ExportResult>(error);
    }
    ExportResult result;
    result.ok = true;
    return result;
}

} // namespace

QFuture<ExportResult> exportPdf(core::Document document, QUrl url, std::vector<std::size_t> pages)
{
    return detail::runAsync([doc = std::move(document), dest = std::move(url),
                             pageList = std::move(pages)]() { return writePdf(doc, dest, pageList); });
}

int countPdfPages(const QByteArray& pdf)
{
    static const QRegularExpression page(QStringLiteral("/Type\\s*/Page(?!s)"));
    int count = 0;
    auto it = page.globalMatch(QString::fromLatin1(pdf));
    while (it.hasNext()) {
        it.next();
        ++count;
    }
    return count;
}

} // namespace actcraft::exporting
