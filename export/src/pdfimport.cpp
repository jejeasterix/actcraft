#include <actcraft/export/pdfimport.h>
#include <actcraft/formats/urlio.h>

#include <QBuffer>
#include <QImage>
#include <QPromise>
#include <QThread>

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <vector>

#ifdef ACTCRAFT_IMPORT_PDF
#include <QtPdf/QPdfDocument>
#endif

namespace actcraft::exporting {

namespace detail {
void noteRenderThread(); // pageexport.cpp : crochet d'observation des tests
} // namespace detail

namespace {

ImportResult failure(const QString& error)
{
    ImportResult result;
    result.error = error;
    return result;
}

// Un fil à nous, pas le groupe de fils : QFuture::result() peut exécuter une tâche encore en file sur le
// fil qui attend (même raison que detail::runAsync de pageexport.cpp).
QFuture<ImportResult> runAsync(std::function<ImportResult()> work)
{
    auto promise = std::make_shared<QPromise<ImportResult>>();
    promise->start();
    QFuture<ImportResult> future = promise->future();
    QThread* thread = QThread::create([promise, work = std::move(work)]() {
        promise->addResult(work());
        promise->finish();
    });
    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
    return future;
}

#ifdef ACTCRAFT_IMPORT_PDF

constexpr qint64 kMaxPagePixels = 100ll * 1000 * 1000; // borne d'une image de page
constexpr int kMinDpi = 36;
constexpr int kMaxDpi = 600;

ImportResult importFrom(const QUrl& url, int dpi)
{
    detail::noteRenderThread();
    dpi = std::clamp(dpi, kMinDpi, kMaxDpi);
    const formats::UrlReadResult read = formats::readUrl(url);
    if (!read.ok) {
        return failure(read.error.isEmpty() ? QStringLiteral("Le fichier PDF n'a pas pu être lu.")
                                            : read.error);
    }
    QByteArray bytes = read.data;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::ReadOnly);
    QPdfDocument pdf;
    pdf.load(&buffer);
    if (pdf.status() != QPdfDocument::Status::Ready) {
        if (pdf.error() == QPdfDocument::Error::IncorrectPassword ||
            pdf.error() == QPdfDocument::Error::UnsupportedSecurityScheme) {
            return failure(QStringLiteral("Ce PDF est protégé : il ne peut pas être importé."));
        }
        return failure(QStringLiteral("Ce fichier n'est pas un PDF lisible."));
    }
    const int count = pdf.pageCount();
    if (count < 1) {
        return failure(QStringLiteral("Ce PDF ne contient aucune page."));
    }
    std::vector<core::Page> pages;
    std::vector<QByteArray> pngs;
    for (int i = 0; i < count; ++i) {
        const QSizeF points = pdf.pagePointSize(i);
        if (points.width() <= 0 || points.height() <= 0) {
            return failure(QStringLiteral("Une page de ce PDF n'a pas de taille."));
        }
        const QSize pixels(std::max(1, qRound(points.width() * dpi / 72.0)),
                           std::max(1, qRound(points.height() * dpi / 72.0)));
        if (static_cast<qint64>(pixels.width()) * pixels.height() > kMaxPagePixels) {
            return failure(QStringLiteral("Une page de ce PDF est trop grande pour être importée."));
        }
        const QImage image = pdf.render(i, pixels);
        if (image.isNull()) {
            return failure(QStringLiteral("Une page de ce PDF n'a pas pu être dessinée."));
        }
        QByteArray png;
        QBuffer out(&png);
        out.open(QIODevice::WriteOnly);
        if (!image.save(&out, "PNG")) {
            return failure(QStringLiteral("Une page de ce PDF n'a pas pu être enregistrée en image."));
        }
        core::Page page;
        page.width = points.width() * 96.0 / 72.0;
        page.height = points.height() * 96.0 / 72.0;
        pages.push_back(std::move(page));
        pngs.push_back(std::move(png));
    }
    ImportResult result;
    result.document = core::Document(std::move(pages));
    for (int i = 0; i < count; ++i) {
        core::Page& page = result.document.page(static_cast<std::size_t>(i));
        const QImage decoded = QImage::fromData(pngs[static_cast<std::size_t>(i)], "PNG");
        core::ImageObject image;
        image.bounds = {0, 0, page.width, page.height};
        image.sourceWidth = decoded.width();
        image.sourceHeight = decoded.height();
        image.resource = result.document.addResource(std::move(pngs[static_cast<std::size_t>(i)]), "png");
        core::PageObject object(image);
        object.common.id = result.document.allocateId();
        object.common.layer = core::Layer::Background;
        object.common.locked = true;
        page.objects.push_back(std::move(object));
    }
    result.ok = true;
    return result;
}

#endif

} // namespace

bool pdfImportAvailable()
{
#ifdef ACTCRAFT_IMPORT_PDF
    return true;
#else
    return false;
#endif
}

QFuture<ImportResult> importPdf(QUrl url, int dpi)
{
#ifdef ACTCRAFT_IMPORT_PDF
    return runAsync([url = std::move(url), dpi]() { return importFrom(url, dpi); });
#else
    static_cast<void>(url);
    static_cast<void>(dpi);
    return runAsync(
        []() { return failure(QStringLiteral("L'import de PDF n'est pas disponible dans cette version.")); });
#endif
}

} // namespace actcraft::exporting
