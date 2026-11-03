#include <actcraft/export/pageexport.h>

#include <QFile>
#include <QImage>
#include <QImageReader>
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

core::Document documentWithARedRect()
{
    core::Shape shape;
    shape.kind = core::ShapeKind::Rectangle;
    shape.bounds = {100, 100, 200, 200};
    shape.fill = 0xFFFF0000u;
    shape.outlineWidth = 0;
    core::Document document;
    core::PageObject object = shape;
    object.common.id = document.allocateId();
    document.page(0).objects.push_back(object);
    return document;
}

} // namespace

class TestExportImage : public QObject
{
    Q_OBJECT
private slots:
    void png1024x768()
    {
        QTemporaryDir dir;
        const QUrl url = QUrl::fromLocalFile(dir.filePath("page.png"));
        const auto result = exporting::exportPageImage(documentWithARedRect(), 0, url).result();
        QVERIFY2(result.ok, qPrintable(result.error));
        const QImage image(url.toLocalFile());
        QCOMPARE(image.size(), QSize(1024, 768));
        QCOMPARE(image.pixelColor(150, 150), QColor(Qt::red));
        QCOMPARE(image.pixelColor(600, 600), QColor(Qt::white));
    }

    void formatFollowsExtension()
    {
        QCOMPARE(exporting::imageFormatFor(QUrl::fromLocalFile("/a/b.PNG")), exporting::ImageFormat::Png);
        QCOMPARE(exporting::imageFormatFor(QUrl::fromLocalFile("/a/b.jpg")), exporting::ImageFormat::Jpeg);
        QCOMPARE(exporting::imageFormatFor(QUrl::fromLocalFile("/a/b.JPEG")), exporting::ImageFormat::Jpeg);
        QCOMPARE(exporting::imageFormatFor(QUrl::fromLocalFile("/a/b.bmp")), exporting::ImageFormat::Bmp);
        QCOMPARE(exporting::imageFormatFor(QUrl::fromLocalFile("/a/b.tif")), exporting::ImageFormat::Tiff);
        QCOMPARE(exporting::imageFormatFor(QUrl::fromLocalFile("/a/b.tiff")), exporting::ImageFormat::Tiff);
        QCOMPARE(exporting::imageFormatFor(QUrl::fromLocalFile("/a/b.xyz")), exporting::ImageFormat::Png);
        QCOMPARE(exporting::imageFormatFor(QUrl::fromLocalFile("/a/b")), exporting::ImageFormat::Png);
    }

    void jpegBmpTiffByExtension()
    {
        const struct
        {
            const char* file;
            const char* format;
        } cases[] = {{"a.jpg", "jpeg"}, {"a.jpeg", "jpeg"}, {"a.bmp", "bmp"}, {"a.tif", "tiff"}, {"a.tiff", "tiff"}};
        for (const auto& c : cases) {
            QTemporaryDir dir;
            const QUrl url = QUrl::fromLocalFile(dir.filePath(c.file));
            const auto result = exporting::exportPageImage(documentWithARedRect(), 0, url).result();
            QVERIFY2(result.ok, c.file);
            QImageReader reader(url.toLocalFile());
            QCOMPARE(QString::fromLatin1(reader.format()), QString::fromLatin1(c.format));
            QCOMPARE(reader.size(), QSize(1024, 768));
            const QImage image = reader.read().convertToFormat(QImage::Format_ARGB32);
            const QColor pixel = image.pixelColor(150, 150);
            QVERIFY2(pixel.red() > 240 && pixel.green() < 20 && pixel.blue() < 20, c.file);
        }
    }

    void pageSizeFollowsThePage()
    {
        core::Document document(500, 300);
        QTemporaryDir dir;
        const QUrl url = QUrl::fromLocalFile(dir.filePath("p.png"));
        QVERIFY(exporting::exportPageImage(document, 0, url).result().ok);
        QCOMPARE(QImage(url.toLocalFile()).size(), QSize(500, 300));
    }

    void unknownPageIsReported()
    {
        QTemporaryDir dir;
        const QUrl url = QUrl::fromLocalFile(dir.filePath("p.png"));
        const auto result = exporting::exportPageImage(core::Document(), 5, url).result();
        QVERIFY(!result.ok);
        QVERIFY(!result.error.isEmpty());
        QVERIFY(!QFile::exists(url.toLocalFile()));
    }

    void unwritablePlaceIsReported()
    {
        QTemporaryDir dir;
        const QUrl url = QUrl::fromLocalFile(dir.filePath("absent/p.png"));
        const auto result = exporting::exportPageImage(core::Document(), 0, url).result();
        QVERIFY(!result.ok);
        QVERIFY(!result.error.isEmpty());
    }

    void runsOffMainThread()
    {
        g_renderThread.store(nullptr);
        exporting::detail::setRenderObserver(&noteThread);
        QTemporaryDir dir;
        const QUrl url = QUrl::fromLocalFile(dir.filePath("p.png"));
        const auto result = exporting::exportPageImage(documentWithARedRect(), 0, url).result();
        exporting::detail::setRenderObserver(nullptr);
        QVERIFY(result.ok);
        QVERIFY(g_renderThread.load() != nullptr);
        QVERIFY(g_renderThread.load() != QThread::currentThread());
    }

    void takesTheCopyNotTheOriginal()
    {
        core::Document document = documentWithARedRect();
        QTemporaryDir dir;
        const QUrl url = QUrl::fromLocalFile(dir.filePath("p.png"));
        auto future = exporting::exportPageImage(document, 0, url);
        document.page(0).objects.clear(); // l'export a sa copie
        QVERIFY(future.result().ok);
        QCOMPARE(QImage(url.toLocalFile()).pixelColor(150, 150), QColor(Qt::red));
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
        text.html = QStringLiteral("<p>texte <b>gras</b></p><img src=\"%1\" width=\"200\" height=\"200\">")
                        .arg(QUrl::fromLocalFile(secretPath).toString())
                        .toStdString();
        core::Document document;
        core::PageObject object = text;
        object.common.id = document.allocateId();
        document.page(0).objects.push_back(object);
        const QUrl url = QUrl::fromLocalFile(dir.filePath("t.png"));
        QVERIFY(exporting::exportPageImage(document, 0, url).result().ok);
        const QImage image = QImage(url.toLocalFile()).convertToFormat(QImage::Format_ARGB32);
        int red = 0;
        int black = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor c = image.pixelColor(x, y);
                red += (c.red() > 200 && c.green() < 60 && c.blue() < 60) ? 1 : 0;
                black += (c.red() < 80 && c.green() < 80 && c.blue() < 80) ? 1 : 0;
            }
        }
        QCOMPARE(red, 0);   // le fichier cité n'a pas été lu
        QVERIFY(black > 20); // mais le texte est bien peint
    }
};

QTEST_MAIN(TestExportImage)
#include "tst_export_image.moc"
