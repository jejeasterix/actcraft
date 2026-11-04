#include <actcraft/export/pdfexport.h>
#include <actcraft/export/pdfimport.h>

#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>

#include <atomic>

using namespace actcraft;

#ifdef ACTCRAFT_IMPORT_PDF
namespace actcraft::exporting::detail {
using RenderObserver = void (*)();
void setRenderObserver(RenderObserver observer);
} // namespace actcraft::exporting::detail
#endif

namespace {

#ifdef ACTCRAFT_IMPORT_PDF
std::atomic<QThread*> g_importThread{nullptr};
void noteThread()
{
    g_importThread.store(QThread::currentThread());
}
#endif

// PDF d'essai produit par l'export de L5-T23 : aucun fichier tiers dans le dépôt.
QUrl writePdf(const QTemporaryDir& dir, int pages, double width = 1024, double height = 768)
{
    core::Document document(width, height);
    for (int i = 1; i < pages; ++i) {
        document.appendPage();
    }
    const QUrl url = QUrl::fromLocalFile(dir.filePath("essai.pdf"));
    if (!exporting::exportPdf(document, url).result().ok) {
        return QUrl();
    }
    return url;
}

} // namespace

class TestExportPdfImport : public QObject
{
    Q_OBJECT
private slots:
    void availabilityFollowsTheBuild()
    {
#ifdef ACTCRAFT_IMPORT_PDF
        QVERIFY(exporting::pdfImportAvailable());
#else
        QVERIFY(!exporting::pdfImportAvailable());
#endif
    }

    void roundTripThreePages()
    {
#ifndef ACTCRAFT_IMPORT_PDF
        QSKIP("Qt PDF absent de cette construction");
#else
        QTemporaryDir dir;
        const QUrl url = writePdf(dir, 3);
        QVERIFY(url.isValid());
        const auto result = exporting::importPdf(url).result();
        QVERIFY2(result.ok, qPrintable(result.error));
        QCOMPARE(result.document.pageCount(), std::size_t{3});
        QVERIFY(core::hasUniqueIds(result.document));
#endif
    }

    void pageSizeFromPdf()
    {
#ifndef ACTCRAFT_IMPORT_PDF
        QSKIP("Qt PDF absent de cette construction");
#else
        QTemporaryDir dir;
        // 1024 × 768 px exportés valent 768 × 576 points ; l'import les rend en pixels (× 96 / 72).
        const auto first = exporting::importPdf(writePdf(dir, 1)).result();
        QVERIFY2(first.ok, qPrintable(first.error));
        QVERIFY(qAbs(first.document.page(0).width - 1024) <= 1.0);
        QVERIFY(qAbs(first.document.page(0).height - 768) <= 1.0);
        QTemporaryDir other;
        const auto second = exporting::importPdf(writePdf(other, 2, 800, 600)).result();
        QVERIFY2(second.ok, qPrintable(second.error));
        QCOMPARE(second.document.pageCount(), std::size_t{2});
        QVERIFY(qAbs(second.document.page(1).width - 800) <= 1.0);
        QVERIFY(qAbs(second.document.page(1).height - 600) <= 1.0);
#endif
    }

    void imageOnBackgroundLocked()
    {
#ifndef ACTCRAFT_IMPORT_PDF
        QSKIP("Qt PDF absent de cette construction");
#else
        QTemporaryDir dir;
        const QUrl url = writePdf(dir, 2);
        const auto result = exporting::importPdf(url, 72).result();
        QVERIFY2(result.ok, qPrintable(result.error));
        QCOMPARE(result.document.pageCount(), std::size_t{2});
        for (std::size_t i = 0; i < result.document.pageCount(); ++i) {
            const core::Page& page = result.document.page(i);
            // Images seulement : aucun objet texte tiré du PDF.
            QCOMPARE(page.objects.size(), std::size_t{1});
            const core::PageObject& object = page.objects.front();
            const auto* image = std::get_if<core::ImageObject>(&object);
            QVERIFY(image != nullptr);
            QCOMPARE(object.common.layer, core::Layer::Background);
            QVERIFY(object.common.locked);
            QVERIFY(object.common.id != 0);
            QCOMPARE(image->bounds.x, 0.0);
            QCOMPARE(image->bounds.y, 0.0);
            QCOMPARE(image->bounds.width, page.width);
            QCOMPARE(image->bounds.height, page.height);
            const core::Resource* resource = result.document.resource(image->resource);
            QVERIFY(resource != nullptr);
            QCOMPARE(QString::fromStdString(image->resource),
                     QString::fromStdString(core::sha256Hex(resource->data)));
            QCOMPARE(resource->extension, std::string("png"));
            const QImage decoded = QImage::fromData(resource->data, "PNG");
            QVERIFY(!decoded.isNull());
            // 72 dpi : un pixel par point, soit 768 × 576 pour une page de 1024 × 768 px.
            QVERIFY(qAbs(decoded.width() - 768) <= 1);
            QVERIFY(qAbs(decoded.height() - 576) <= 1);
            QCOMPARE(image->sourceWidth, decoded.width());
            QCOMPARE(image->sourceHeight, decoded.height());
        }
        const auto dense = exporting::importPdf(url, 144).result();
        QVERIFY2(dense.ok, qPrintable(dense.error));
        const auto* denseImage = std::get_if<core::ImageObject>(&dense.document.page(0).objects.front());
        QVERIFY(denseImage != nullptr);
        QVERIFY(qAbs(denseImage->sourceWidth - 1536) <= 1);
#endif
    }

    void offMainThread()
    {
#ifndef ACTCRAFT_IMPORT_PDF
        QSKIP("Qt PDF absent de cette construction");
#else
        QTemporaryDir dir;
        const QUrl url = writePdf(dir, 1);
        g_importThread.store(nullptr);
        exporting::detail::setRenderObserver(&noteThread);
        const auto result = exporting::importPdf(url).result();
        exporting::detail::setRenderObserver(nullptr);
        QVERIFY2(result.ok, qPrintable(result.error));
        QVERIFY(g_importThread.load() != nullptr);
        QVERIFY(g_importThread.load() != QThread::currentThread());
#endif
    }

    void unreadableFileIsReported()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("faux.pdf");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("ceci n'est pas un PDF");
        file.close();
        const auto garbage = exporting::importPdf(QUrl::fromLocalFile(path)).result();
        QVERIFY(!garbage.ok);
        QVERIFY(!garbage.error.isEmpty());
        const auto missing = exporting::importPdf(QUrl::fromLocalFile(dir.filePath("absent.pdf"))).result();
        QVERIFY(!missing.ok);
        QVERIFY(!missing.error.isEmpty());
    }

    void absentWithoutQtPdf()
    {
#ifdef ACTCRAFT_IMPORT_PDF
        QSKIP("Qt PDF présent");
#else
        QVERIFY(!exporting::pdfImportAvailable());
        QTemporaryDir dir;
        const QUrl url = writePdf(dir, 1);
        QVERIFY(url.isValid());
        const auto result = exporting::importPdf(url).result();
        QVERIFY(!result.ok);
        QVERIFY(!result.error.isEmpty());
        QCOMPARE(result.document.pageCount(), std::size_t{1}); // document vierge, jamais vide
#endif
    }
};

QTEST_GUILESS_MAIN(TestExportPdfImport)
#include "tst_export_pdfimport.moc"
