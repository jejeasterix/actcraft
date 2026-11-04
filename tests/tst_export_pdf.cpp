#include <actcraft/export/pdfexport.h>

#include <QFile>
#include <QImage>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>

#include <atomic>

using namespace actcraft;

namespace actcraft::exporting::detail {
using RenderObserver = void (*)();
void setRenderObserver(RenderObserver observer);
} // namespace actcraft::exporting::detail

namespace {

std::atomic<QThread*> g_renderThread{nullptr};
void noteThread()
{
    g_renderThread.store(QThread::currentThread());
}

core::Document documentOfPages(int count)
{
    core::Document document;
    for (int i = 1; i < count; ++i) {
        document.appendPage();
    }
    return document;
}

QByteArray exportedBytes(const core::Document& document, const QString& path,
                         std::vector<std::size_t> pages = {})
{
    const auto result = exporting::exportPdf(document, QUrl::fromLocalFile(path), std::move(pages)).result();
    if (!result.ok) {
        return QByteArray();
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    return file.readAll();
}

} // namespace

class TestExportPdf : public QObject
{
    Q_OBJECT
private slots:
    void threePagesGiveThreePages()
    {
        QTemporaryDir dir;
        const QUrl url = QUrl::fromLocalFile(dir.filePath("doc.pdf"));
        QVERIFY(exporting::exportPdf(documentOfPages(3), url).result().ok);
        QFile file(url.toLocalFile());
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray pdf = file.readAll();
        QVERIFY(pdf.startsWith("%PDF-"));
        QCOMPARE(exporting::countPdfPages(pdf), 3);
    }

    void countCountsPageDictionariesOnly()
    {
        QCOMPARE(
            exporting::countPdfPages("<< /Type /Pages /Kids [1 0 R] >> << /Type /Page >> << /Type/Page >>"),
            2);
        QCOMPARE(exporting::countPdfPages(QByteArray()), 0);
    }

    void selectedPagesOnly()
    {
        QTemporaryDir dir;
        const QByteArray pdf = exportedBytes(documentOfPages(4), dir.filePath("s.pdf"), {1, 3});
        QCOMPARE(exporting::countPdfPages(pdf), 2);
    }

    void pageSizeInPoints()
    {
        QTemporaryDir dir;
        const QByteArray pdf = exportedBytes(core::Document(1024, 768), dir.filePath("s.pdf"));
        const QRegularExpression box(
            QStringLiteral("/MediaBox\\s*\\[\\s*0\\s+0\\s+([0-9.]+)\\s+([0-9.]+)\\s*\\]"));
        const auto match = box.match(QString::fromLatin1(pdf));
        QVERIFY2(match.hasMatch(), "MediaBox absente");
        QVERIFY2(qAbs(match.captured(1).toDouble() - 768.0) < 0.5, qPrintable(match.captured(1)));
        QVERIFY2(qAbs(match.captured(2).toDouble() - 576.0) < 0.5, qPrintable(match.captured(2)));
    }

    void eachPageKeepsItsOwnSize()
    {
        core::Document document(1024, 768);
        document.appendPage();
        document.page(1).width = 400;
        document.page(1).height = 800;
        QTemporaryDir dir;
        const QByteArray pdf = exportedBytes(document, dir.filePath("s.pdf"));
        const QRegularExpression box(
            QStringLiteral("/MediaBox\\s*\\[\\s*0\\s+0\\s+([0-9.]+)\\s+([0-9.]+)\\s*\\]"));
        auto it = box.globalMatch(QString::fromLatin1(pdf));
        QVERIFY(it.hasNext());
        it.next();
        QVERIFY(it.hasNext());
        const auto second = it.next();
        QVERIFY2(qAbs(second.captured(1).toDouble() - 300.0) < 0.5, qPrintable(second.captured(1)));
        QVERIFY2(qAbs(second.captured(2).toDouble() - 600.0) < 0.5, qPrintable(second.captured(2)));
    }

    void unknownPageIsReported()
    {
        QTemporaryDir dir;
        const QUrl url = QUrl::fromLocalFile(dir.filePath("s.pdf"));
        const auto result = exporting::exportPdf(documentOfPages(2), url, {5}).result();
        QVERIFY(!result.ok);
        QVERIFY(!result.error.isEmpty());
        QVERIFY(!QFile::exists(url.toLocalFile()));
    }

    void runsOffMainThread()
    {
        g_renderThread.store(nullptr);
        exporting::detail::setRenderObserver(&noteThread);
        QTemporaryDir dir;
        const auto result =
            exporting::exportPdf(documentOfPages(2), QUrl::fromLocalFile(dir.filePath("s.pdf"))).result();
        exporting::detail::setRenderObserver(nullptr);
        QVERIFY(result.ok);
        QVERIFY(g_renderThread.load() != nullptr);
        QVERIFY(g_renderThread.load() != QThread::currentThread());
    }

    void textIsGuarded()
    {
        QTemporaryDir dir;
        QImage secret(200, 200, QImage::Format_ARGB32);
        secret.fill(Qt::red);
        const QString secretPath = dir.filePath("secret.png");
        QVERIFY(secret.save(secretPath));
        core::Text text;
        text.bounds = {50, 50, 400, 300};
        text.html = QStringLiteral("<p>texte</p><img src=\"%1\" width=\"200\" height=\"200\">")
                        .arg(QUrl::fromLocalFile(secretPath).toString())
                        .toStdString();
        core::Document document;
        core::PageObject object = text;
        object.common.id = document.allocateId();
        document.page(0).objects.push_back(object);
        const QByteArray pdf = exportedBytes(document, dir.filePath("t.pdf"));
        QVERIFY(!pdf.isEmpty());
        QVERIFY2(!pdf.contains("/Subtype /Image"), "le fichier cité a été lu et dessiné");
    }
};

QTEST_MAIN(TestExportPdf)
#include "tst_export_pdf.moc"
