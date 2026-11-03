#include "asyncrun.h"

#include <actcraft/export/pageexport.h>
#include <actcraft/formats/urlio.h>
#include <actcraft/render/pagepainter.h>

#include <QBuffer>
#include <QImage>
#include <QPromise>
#include <QString>
#include <QThread>

#include <atomic>
#include <functional>
#include <memory>

namespace actcraft::exporting {

namespace detail {
// Crochet de test (L5-T23) : appelé sur le fil qui rend, pour prouver que le rendu n'est pas sur le fil
// principal. Aucun en production.
using RenderObserver = void (*)();
void setRenderObserver(RenderObserver observer);
void noteRenderThread();
} // namespace detail

namespace {
std::atomic<detail::RenderObserver> g_observer{nullptr};
} // namespace

namespace detail {
void setRenderObserver(RenderObserver observer)
{
    g_observer.store(observer);
}
void noteRenderThread()
{
    if (const RenderObserver observer = g_observer.load()) {
        observer();
    }
}
} // namespace detail

ImageFormat imageFormatFor(const QUrl& url)
{
    const QString path = url.path().toLower();
    if (path.endsWith(QStringLiteral(".jpg")) || path.endsWith(QStringLiteral(".jpeg"))) {
        return ImageFormat::Jpeg;
    }
    if (path.endsWith(QStringLiteral(".bmp"))) {
        return ImageFormat::Bmp;
    }
    if (path.endsWith(QStringLiteral(".tif")) || path.endsWith(QStringLiteral(".tiff"))) {
        return ImageFormat::Tiff;
    }
    return ImageFormat::Png;
}

namespace {

const char* formatName(ImageFormat format)
{
    switch (format) {
    case ImageFormat::Jpeg:
        return "JPEG";
    case ImageFormat::Bmp:
        return "BMP";
    case ImageFormat::Tiff:
        return "TIFF";
    case ImageFormat::Png:
        break;
    }
    return "PNG";
}

ExportResult renderImage(const core::Document& document, std::size_t page, const QUrl& url)
{
    if (page >= document.pageCount()) {
        return detail::failure<ExportResult>(QStringLiteral("Cette page n'existe pas."));
    }
    detail::noteRenderThread();
    const render::DecodedImages images = render::decodeImages(document);
    const QImage image = render::renderPage(document.page(page), images.lookup(), 1.0);
    if (image.isNull()) {
        return detail::failure<ExportResult>(
            QStringLiteral("La page est trop grande pour être exportée en image."));
    }
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, formatName(imageFormatFor(url)))) {
        return detail::failure<ExportResult>(
            QStringLiteral("Le format d'image demandé n'est pas disponible."));
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

QFuture<ExportResult> exportPageImage(core::Document document, std::size_t page, QUrl url)
{
    return detail::runAsync([document = std::move(document), page, url = std::move(url)]() {
        return renderImage(document, page, url);
    });
}

} // namespace actcraft::exporting
