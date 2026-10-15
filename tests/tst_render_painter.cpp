#include <actcraft/core/document.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/render/painternode.h>

#include <QBuffer>
#include <QElapsedTimer>
#include <QFile>
#include <QFont>
#include <QFontMetricsF>
#include <QImageReader>
#include <QPainter>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <QUrl>

#include <algorithm>
#include <functional>
#include <memory>

using namespace actcraft;

namespace {

bool near(QColor actual, QColor expected, int tolerance)
{
    return qAbs(actual.red() - expected.red()) <= tolerance &&
           qAbs(actual.green() - expected.green()) <= tolerance &&
           qAbs(actual.blue() - expected.blue()) <= tolerance;
}

core::PageObject square(double x, double y, double size, core::Argb fill)
{
    core::Shape s;
    s.kind = core::ShapeKind::Path;
    s.paths = {{{0, 0}, {size, 0}, {size, size}, {0, size}}};
    s.pen.style = core::PenStyle::None;
    s.brush = core::solidFill(fill);
    core::PageObject o = s;
    o.common.transform = core::translation(x, y);
    return o;
}

const render::ImageLookup kNoImages = [](const std::string&) { return QImage(); };

int countPixels(const QImage& image, const QRect& area, const std::function<bool(QColor)>& match)
{
    int n = 0;
    const QRect r = area.intersected(image.rect());
    for (int y = r.top(); y <= r.bottom(); ++y) {
        for (int x = r.left(); x <= r.right(); ++x) {
            n += match(image.pixelColor(x, y)) ? 1 : 0;
        }
    }
    return n;
}

bool isRed(QColor c)
{
    return c.red() > 200 && c.green() < 80 && c.blue() < 80;
}

bool isDark(QColor c)
{
    return c.red() < 80 && c.green() < 80 && c.blue() < 80;
}

bool isLight(QColor c)
{
    return c.red() > 200 && c.green() > 200 && c.blue() > 200;
}

bool darkBetween(const QImage& image, int x0, int x1, int y)
{
    return countPixels(image, QRect(x0, y, x1 - x0 + 1, 1), isDark) > 0;
}

// Encre (pixels sombres) d'un run noir de 24 pt, ligne de base à 40 px, recalé sur `width` (0 : pas de
// recalage).
QRect runInk(const QString& text, double width)
{
    core::Text t;
    t.bounds = {0, 0, 600, 60};
    core::TextSpan run;
    run.text = text.toStdString();
    run.fontFamily = "Arial";
    run.pointSize = 24;
    run.baseline = {0, 40};
    run.bounds = {0, 0, width, 0};
    t.runs.push_back(run);
    core::PageObject o = t;
    o.common.transform = core::translation(50, 50);
    core::Page page;
    page.objects.push_back(o);
    const QImage image = render::renderPage(page, kNoImages, 1.0);
    QRect ink;
    for (int y = 0; y < 200; ++y) {
        for (int x = 0; x < 1000; ++x) {
            if (isDark(image.pixelColor(x, y))) {
                ink |= QRect(x, y, 1, 1);
            }
        }
    }
    return ink;
}

core::Page gridPage(core::GridLevel level)
{
    level.spacingX = 100;
    level.spacingY = 100;
    core::Page page;
    page.grid.visible = true;
    page.grid.levels.push_back(level);
    return page;
}

} // namespace

// Peintre partagé (DA5) : règles de dessin contrôlées au pixel sur des pages construites ici.
class TstRenderPainter : public QObject
{
    Q_OBJECT

private slots:
    void blankPageIsWhiteAtTheGivenScale()
    {
        const QImage image = render::renderPage(core::Page{}, kNoImages, 0.5);
        QCOMPARE(image.size(), QSize(512, 384));
        QVERIFY(near(image.pixelColor(10, 10), Qt::white, 0));
    }

    void pathShapeFollowsItsTransform()
    {
        core::Page page;
        page.objects.push_back(square(100, 50, 40, 0xFFFF0000u));
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        QVERIFY(near(image.pixelColor(120, 70), Qt::red, 2));
        QVERIFY(near(image.pixelColor(90, 70), Qt::white, 2));
        QVERIFY(near(image.pixelColor(150, 70), Qt::white, 2));
    }

    void rotationIsClockwise()
    {
        core::Page page;
        core::PageObject o = square(0, 0, 40, 0xFF0000FFu);
        o.common.transform = core::compose(core::rotation(90), core::translation(200, 200));
        page.objects.push_back(o);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        // (10, 10) → rotation de 90° → (−10, 10) → (190, 210)
        QVERIFY(near(image.pixelColor(190, 210), Qt::blue, 2));
        QVERIFY(near(image.pixelColor(210, 210), Qt::white, 2));
    }

    void translucencyAndVisibility()
    {
        core::Page page;
        core::PageObject half = square(0, 0, 40, 0xFF000000u);
        half.common.translucency = 128;
        page.objects.push_back(half);
        core::PageObject hidden = square(100, 0, 40, 0xFF000000u);
        hidden.common.visible = false;
        page.objects.push_back(hidden);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        const int grey = image.pixelColor(20, 20).red();
        QVERIFY2(grey > 110 && grey < 145, qPrintable(QString::number(grey)));
        QVERIFY(near(image.pixelColor(120, 20), Qt::white, 0));
    }

    void highlighterStrokeHasRoundCapsAndAlpha()
    {
        core::Stroke s;
        s.kind = core::StrokeKind::Highlighter;
        s.cap = core::PenCap::Round;
        s.baseWidth = 20;
        s.color = 0x80FFFF00u;
        s.points = {{100, 100, 0.5f}, {200, 100, 0.5f}};
        core::Page page;
        page.objects.push_back(s);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        QVERIFY(near(image.pixelColor(150, 100), QColor(255, 255, 127),
                     6));                                         // jaune à moitié transparent sur blanc
        QVERIFY(!near(image.pixelColor(93, 100), Qt::white, 10)); // bout rond au-delà du premier point
    }

    void penStrokeKeepsSquareSegments()
    {
        core::Stroke s;
        s.baseWidth = 10;
        s.points = {{100, 100, 0.5f}, {200, 100, 0.5f}};
        core::Page page;
        page.objects.push_back(s);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        QVERIFY(near(image.pixelColor(97, 102), Qt::black, 30)); // bout carré : coin rempli
    }

    // Patron écarté : le stylo d'ActCraft peint comme un trait lu (un seul chemin, plume uniforme de
    // baseWidth). Le peintre doit reprendre la géométrie du moteur du prototype (StrokePainterNode →
    // paintStrokeSegments) : un segment par paire de points, largeur strokeWidthAt(baseWidth, pression du
    // premier point).
    void penStrokeMatchesThePrototypeEngine()
    {
        core::Stroke s;
        s.baseWidth = 10;
        s.color = 0xFF000000u;
        s.points = {{100, 100, 1.0f}, {200, 100, 0.0f}, {300, 100, 0.0f}}; // 15 px puis 5 px
        core::Page page;
        page.objects.push_back(s);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        QVERIFY(near(image.pixelColor(150, 106), Qt::black, 30)); // segment épais : demi-largeur 7,5
        QVERIFY(near(image.pixelColor(250, 106), Qt::white, 0));  // segment fin : demi-largeur 2,5

        QImage reference(image.size(), QImage::Format_ARGB32_Premultiplied);
        reference.fill(Qt::white);
        QPainter painter(&reference);
        painter.setRenderHint(QPainter::Antialiasing, true); // comme PainterNode::render
        render::paintStrokeSegments(painter, s.points, s.baseWidth, QColor::fromRgba(s.color));
        painter.end();
        QCOMPARE(image, reference);
    }

    // Formes du prototype : couleurs fill/outline et opacité de la forme (ShapePainterNode), pas pen/brush.
    void prototypeShapeUsesItsOwnColours()
    {
        core::Shape shape;
        shape.kind = core::ShapeKind::Rectangle;
        shape.bounds = {10, 10, 100, 100};
        shape.fill = 0xFFFF0000u;
        shape.outlineWidth = 0;
        shape.opacity = 0.5;
        core::Page page;
        page.objects.push_back(shape);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        QVERIFY(near(image.pixelColor(60, 60), QColor(255, 128, 128), 3));
        QVERIFY(near(image.pixelColor(5, 60), Qt::white, 0));
    }

    void imageResourceWithKeyColor()
    {
        QImage source(4, 4, QImage::Format_ARGB32);
        source.fill(Qt::white);
        source.setPixelColor(0, 0, Qt::green);
        render::DecodedImages images;
        images.insert("cle", source);
        core::ImageObject im;
        im.bounds = {0, 0, 4, 4};
        im.resource = "cle";
        im.hasKeyColor = true;
        im.keyColor = 0xFFFFFFFFu;
        core::PageObject o = im;
        o.common.transform = core::scaling(25, 25); // 100 × 100 px
        core::Page page;
        page.background.fill = core::solidFill(0xFF0000FFu);
        page.objects.push_back(o);
        const QImage image = render::renderPage(page, images.lookup(), 1.0);
        QVERIFY(near(image.pixelColor(10, 10), Qt::green, 30));
        QVERIFY(near(image.pixelColor(80, 80), Qt::blue, 30)); // le blanc de l'image laisse voir le fond
    }

    void missingImageIsMarked()
    {
        core::ImageObject im;
        im.bounds = {0, 0, 50, 50};
        im.resource = "absente";
        core::Page page;
        page.objects.push_back(im);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        QVERIFY(!near(image.pixelColor(25, 25), Qt::white, 5));
    }

    void htmlTextIsDrawnInItsBounds()
    {
        core::Text t;
        t.bounds = {0, 0, 300, 60};
        t.html = "<p style=\"color:#ff0000; font-size:40px\">XXXX</p>";
        core::PageObject o = t;
        o.common.transform = core::translation(100, 100);
        core::Page page;
        page.objects.push_back(o);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        int red = 0;
        for (int y = 100; y < 160; ++y) {
            for (int x = 100; x < 400; ++x) {
                const QColor c = image.pixelColor(x, y);
                red += c.red() > 200 && c.green() < 80 ? 1 : 0;
            }
        }
        QVERIFY2(red > 200, qPrintable(QString::number(red)));
        QVERIFY(near(image.pixelColor(50, 50), Qt::white, 0));
    }

    void runsAreDrawnAtTheirBaseline()
    {
        core::Text t;
        t.bounds = {0, 0, 200, 40};
        core::TextSpan run;
        run.text = "XXXX";
        run.fontFamily = "Arial";
        run.pointSize = 24;
        run.color = 0xFF0000FFu;
        run.baseline = {0, 30};
        run.bounds = {0, 0, 0, 0}; // pas de recalage
        t.runs.push_back(run);
        core::PageObject o = t;
        o.common.transform = core::translation(100, 100);
        core::Page page;
        page.objects.push_back(o);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        int blue = 0;
        for (int y = 100; y < 131; ++y) {
            for (int x = 100; x < 220; ++x) {
                blue += image.pixelColor(x, y).blue() > 200 && image.pixelColor(x, y).red() < 80 ? 1 : 0;
            }
        }
        QVERIFY2(blue > 100, qPrintable(QString::number(blue)));
        QVERIFY(near(image.pixelColor(150, 140), Qt::white, 0)); // rien sous la ligne de base
    }

    void backgroundGradientAndPattern()
    {
        core::Page page;
        page.background.fill.kind = core::FillKind::Gradient;
        page.background.fill.color = 0xFF000000u;
        page.background.fill.color2 = 0xFFFFFFFFu;
        page.background.fill.gradient = core::GradientKind::Vertical;
        const QImage image = render::renderPage(page, kNoImages, 0.25);
        QVERIFY(image.pixelColor(100, 2).red() < 20);
        QVERIFY(image.pixelColor(100, 189).red() > 235);
        page.background.fill.gradient = core::GradientKind::Horizontal;
        const QImage horizontal = render::renderPage(page, kNoImages, 0.25);
        QVERIFY(horizontal.pixelColor(2, 100).red() < 20);
        QVERIFY(horizontal.pixelColor(253, 100).red() > 235);
    }

    void pictureBackgroundCoversThePage()
    {
        QImage source(2, 2, QImage::Format_ARGB32);
        source.fill(Qt::red);
        render::DecodedImages images;
        images.insert("fond", source);
        core::Page page;
        page.background.image = "fond";
        const QImage image = render::renderPage(page, images.lookup(), 0.25);
        QVERIFY(near(image.pixelColor(5, 5), Qt::red, 2));
        QVERIFY(near(image.pixelColor(250, 186), Qt::red, 2));
    }

    void gridLinesFollowTheStep()
    {
        core::Page page;
        page.grid.visible = true;
        core::GridLevel level;
        level.spacingX = 100;
        level.spacingY = 100;
        level.color = 0xFF000000u;
        page.grid.levels.push_back(level);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        // Trait de 1 px non lissé en x = floor(100 + 0,2) = 100.
        QVERIFY(near(image.pixelColor(100, 50), Qt::black, 10));
        QVERIFY(near(image.pixelColor(150, 50), Qt::white, 0));
        QVERIFY(near(image.pixelColor(50, 100), Qt::black, 10));
        page.grid.visible = false;
        QVERIFY(near(render::renderPage(page, kNoImages, 1.0).pixelColor(100, 50), Qt::white, 0));
    }

    void gridMarksAtIntersections()
    {
        core::Page page;
        page.grid.visible = true;
        core::GridLevel level;
        level.spacingX = 100;
        level.spacingY = 100;
        level.line = core::GridLine::Cross;
        page.grid.levels.push_back(level);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        QVERIFY(near(image.pixelColor(100, 100), Qt::black, 10));
        QVERIFY(near(image.pixelColor(102, 100), Qt::black, 10));
        QVERIFY(near(image.pixelColor(150, 100), Qt::white, 0));
    }

    void thumbnailIsA320PixelWidePng()
    {
        core::Page page;
        page.objects.push_back(square(0, 0, 512, 0xFFFF0000u));
        const QByteArray png = render::thumbnailPng(page, kNoImages);
        QVERIFY(png.startsWith("\x89PNG"));
        QImage image;
        QVERIFY(image.loadFromData(png, "PNG"));
        QCOMPARE(image.size(), QSize(320, 240));
        QVERIFY(near(image.pixelColor(40, 40), Qt::red, 4));
    }

    void decodeImagesReadsEveryResource()
    {
        core::Document document;
        QImage source(3, 3, QImage::Format_ARGB32);
        source.fill(Qt::blue);
        QByteArray png;
        QBuffer buffer(&png);
        buffer.open(QIODevice::WriteOnly);
        source.save(&buffer, "PNG");
        const std::string key = document.addResource(png, "ird"); // PNG renommé : la signature fait foi
        document.addResource(QByteArray("ID3 son"), "mp3");
        const render::DecodedImages images = render::decodeImages(document);
        QCOMPARE(images.size(), std::size_t{1});
        QCOMPARE(images.find(key).size(), QSize(3, 3));
        QVERIFY(images.find("absente").isNull());
    }

    void paintsOffTheMainThread()
    {
        // Comme les miniatures (L2-T17) : formes ET texte (polices, QTextDocument) peints sur un fil
        // secondaire.
        core::Page page;
        page.objects.push_back(square(0, 0, 100, 0xFF00FF00u));
        core::Text t;
        t.bounds = {0, 0, 300, 60};
        t.html = "<p style=\"color:#ff0000; font-size:40px\">XXXX</p>";
        core::PageObject text = t;
        text.common.transform = core::translation(200, 200);
        page.objects.push_back(text);
        QThread* worker = nullptr;
        QImage image;
        // QThread::create, jamais QFuture::result() : il peut exécuter la tâche dans le fil appelant.
        const std::unique_ptr<QThread> thread(QThread::create([&page, &worker, &image] {
            worker = QThread::currentThread();
            image = render::renderPage(page, kNoImages, 1.0);
        }));
        thread->start();
        QVERIFY(thread->wait(60000));
        QVERIFY(worker != nullptr);
        QVERIFY(worker != QThread::currentThread());
        QVERIFY(near(image.pixelColor(20, 20), Qt::green, 2));
        int red = 0;
        for (int y = 200; y < 260; ++y) {
            for (int x = 200; x < 500; ++x) {
                red += image.pixelColor(x, y).red() > 200 && image.pixelColor(x, y).green() < 80 ? 1 : 0;
            }
        }
        QVERIFY2(red > 200, qPrintable(QString::number(red)));
    }

    // --- Revue adversariale
    // -------------------------------------------------------------------------------------

    // Le HTML vient des fichiers ouverts : aucune ressource externe (image, fond, feuille de style) n'est
    // chargée.
    void htmlLoadsNoExternalResource()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QImage red(60, 60, QImage::Format_ARGB32);
        red.fill(Qt::red);
        const QString png = dir.filePath(QStringLiteral("rouge.png"));
        QVERIFY(red.save(png, "PNG"));
        const QString css = dir.filePath(QStringLiteral("style.css"));
        {
            QFile file(css);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("p { color: #ff0000; background-color: #ff0000; }");
        }
        const QString pngUrl = QUrl::fromLocalFile(png).toString();
        const QString cssUrl = QUrl::fromLocalFile(css).toString();
        const QStringList variants = {
            QStringLiteral("<img src=\"") + pngUrl + QStringLiteral("\" width=\"60\" height=\"60\">"),
            QStringLiteral("<img src=\"") + png + QStringLiteral("\" width=\"60\" height=\"60\">"),
            QStringLiteral("<img src=\"") + pngUrl + QStringLiteral("\">"),
            QStringLiteral("<table background=\"") + pngUrl +
                QStringLiteral("\" width=\"200\" height=\"60\"><tr><td>XXXX</td></tr></table>"),
            QStringLiteral("<p style=\"background-image:url(") + pngUrl + QStringLiteral(")\">XXXX XXXX</p>"),
            QStringLiteral("<link rel=\"stylesheet\" href=\"") + cssUrl + QStringLiteral("\"><p>XXXX</p>"),
            QStringLiteral("<style>@import url(") + cssUrl + QStringLiteral(");</style><p>XXXX</p>"),
        };
        for (const QString& html : variants) {
            core::Text t;
            t.bounds = {0, 0, 300, 100};
            t.html = html.toStdString();
            core::PageObject o = t;
            o.common.transform = core::translation(50, 50);
            core::Page page;
            page.objects.push_back(o);
            const QImage image = render::renderPage(page, kNoImages, 1.0);
            const int reds = countPixels(image, QRect(0, 0, 450, 250), isRed);
            QVERIFY2(reds == 0, qPrintable(html + QStringLiteral(" : ") + QString::number(reds)));
        }
    }

    void tallPageThumbnailIsBounded()
    {
        core::Page page;
        page.width = 100;
        page.height = 100000;
        page.objects.push_back(square(0, 0, 100, 0xFFFF0000u));
        QElapsedTimer timer;
        timer.start();
        QImage image;
        QVERIFY(image.loadFromData(render::thumbnailPng(page, kNoImages), "PNG"));
        QVERIFY2(timer.elapsed() < 3000, qPrintable(QString::number(timer.elapsed())));
        QCOMPARE(image.height(), render::kThumbnailMaxHeight); // largeur réduite d'autant (2,4 px)
        QVERIFY(image.width() >= 1 && image.width() <= 3);
        page.width = 10;
        QVERIFY(image.loadFromData(render::thumbnailPng(page, kNoImages), "PNG"));
        QCOMPARE(image.size(), QSize(1, render::kThumbnailMaxHeight));
        page.width = 2000;
        page.height = 100;
        QVERIFY(image.loadFromData(render::thumbnailPng(page, kNoImages), "PNG"));
        QCOMPARE(image.size(), QSize(render::kThumbnailWidth, 16));
    }

    void degeneratePageThumbnailIsBlank()
    {
        for (const QSizeF size : {QSizeF(0, 768), QSizeF(-5, 768), QSizeF(1024, 0), QSizeF(qQNaN(), 768)}) {
            core::Page page;
            page.width = size.width();
            page.height = size.height();
            QImage image;
            QVERIFY(image.loadFromData(render::thumbnailPng(page, kNoImages), "PNG"));
            QCOMPARE(image.size(), QSize(render::kThumbnailWidth, 240));
            QVERIFY(near(image.pixelColor(160, 120), Qt::white, 0));
        }
    }

    // Référence (render_flipchart.py) : largeur mesurée sur le texte sans ses espaces de fin (rstrip).
    void runLeadingSpacesCountInTheMeasuredWidth()
    {
        QImage device(1, 1, QImage::Format_ARGB32_Premultiplied);
        device.setDotsPerMeterX(3780);
        device.setDotsPerMeterY(3780);
        QFont font(QStringLiteral("Arial"));
        font.setPointSizeF(24);
        const QString text = QStringLiteral("    XXXXXXXX  ");
        const double advance = QFontMetricsF(font, &device).horizontalAdvance(QStringLiteral("    XXXXXXXX"));
        const QRect natural = runInk(text, 0);
        const QRect measured = runInk(text, advance);
        QVERIFY(!natural.isEmpty());
        QVERIFY2(qAbs(measured.width() - natural.width()) <= 2,
                 qPrintable(QString::number(natural.width()) + QStringLiteral(" → ") +
                            QString::number(measured.width())));
    }

    void runFontIsRefittedToTheMeasuredWidth()
    {
        const QRect natural = runInk(QStringLiteral("XXXXXXXX"), 0);
        // 24 pt à 96 ppp = 32 px : hauteur de capitale d'environ 23 px (17 px à 72 ppp).
        QVERIFY2(natural.height() >= 19 && natural.height() <= 30,
                 qPrintable(QString::number(natural.height())));
        const QRect wider = runInk(QStringLiteral("XXXXXXXX"), 1.5 * natural.width());
        QVERIFY2(wider.width() > 1.3 * natural.width(),
                 qPrintable(QString::number(natural.width()) + QStringLiteral(" → ") +
                            QString::number(wider.width())));
    }

    void gridMarksRoundHalfToEven()
    {
        // grid.py : round() de Python (au pair) ; y = 100,5 → ligne 100.
        core::GridLevel level;
        level.line = core::GridLine::Cross;
        level.start = {0, 0.5};
        const QImage image = render::renderPage(gridPage(level), kNoImages, 1.0);
        QVERIFY(near(image.pixelColor(102, 100), Qt::black, 10));
        QVERIFY(near(image.pixelColor(102, 101), Qt::white, 0));
    }

    void gridLinesAreBoundedPerPage()
    {
        core::Page page;
        page.width = 100000;
        page.height = 100000;
        page.grid.visible = true;
        for (int i = 0; i < 20; ++i) {
            core::GridLevel level;
            level.spacingX = 0.001;
            level.spacingY = 0.001;
            page.grid.levels.push_back(level);
        }
        QElapsedTimer timer;
        timer.start();
        const QImage image = render::renderPage(page, kNoImages, 0.01);
        QVERIFY2(timer.elapsed() < 3000, qPrintable(QString::number(timer.elapsed())));
        QCOMPARE(image.size(), QSize(1000, 1000));
    }

    void decodeImagesCanBoundTheirSize()
    {
        core::Document document;
        QImage source(400, 200, QImage::Format_ARGB32);
        source.fill(Qt::blue);
        QByteArray png;
        QBuffer buffer(&png);
        buffer.open(QIODevice::WriteOnly);
        source.save(&buffer, "PNG");
        const std::string key = document.addResource(png, "png");
        QCOMPARE(render::decodeImages(document).find(key).size(), QSize(400, 200));
        const render::DecodedImages small = render::decodeImages(document, 100);
        QCOMPARE(small.find(key).size(), QSize(100, 50));
        QVERIFY(near(small.find(key).pixelColor(50, 25), Qt::blue, 2));
        QCOMPARE(render::decodeImages(document, 1000).find(key).size(), QSize(400, 200)); // jamais agrandie
    }

    void objectTransformComposesWithPageScale()
    {
        core::Page page;
        page.objects.push_back(square(100, 50, 40, 0xFFFF0000u));
        const QImage image = render::renderPage(page, kNoImages, 0.5);
        QVERIFY(near(image.pixelColor(60, 35), Qt::red, 2));
        QVERIFY(near(image.pixelColor(110, 35), Qt::white, 2));
    }

    // Traits lus à bouts ronds (surligneur, stylo) : jamais les segments carrés du moteur du prototype.
    void readStrokesKeepRoundCaps()
    {
        core::Stroke highlighter;
        highlighter.kind = core::StrokeKind::Highlighter;
        highlighter.cap = core::PenCap::Round;
        highlighter.baseWidth = 20;
        highlighter.color = 0xFF00FF00u;
        highlighter.points = {{100, 100, 0.5f}, {200, 100, 0.5f}};
        core::Stroke annotation = highlighter;
        annotation.kind = core::StrokeKind::Pen;
        annotation.color = 0xFF0000FFu;
        annotation.points = {{100, 300, 0.5f}, {200, 300, 0.5f}};
        core::Page page;
        page.objects.push_back(highlighter);
        page.objects.push_back(annotation);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        QVERIFY(near(image.pixelColor(150, 100), Qt::green, 2));
        QVERIFY(near(image.pixelColor(150, 300), Qt::blue, 2));
        QVERIFY(!near(image.pixelColor(92, 100), Qt::white, 10)); // bout rond
        QVERIFY(near(image.pixelColor(91, 91), Qt::white, 2));    // coin vide (bout carré : couvert)
        QVERIFY(near(image.pixelColor(91, 291), Qt::white, 2));
    }

    void backgroundPatternIsDrawn()
    {
        core::Page page;
        page.background.fill.kind = core::FillKind::Pattern;
        page.background.fill.color = 0xFF000000u;
        page.background.fill.pattern = 6; // Qt::Dense4Pattern : un pixel sur deux
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        const QRect area(100, 100, 20, 20);
        QVERIFY(countPixels(image, area, isDark) > 100);
        QVERIFY(countPixels(image, area, isLight) > 100);
    }

    void gridIsBelowObjectsUnlessDrawnAbove()
    {
        core::Page page = gridPage(core::GridLevel{});
        page.objects.push_back(square(80, 30, 50, 0xFFFF0000u));
        QVERIFY(near(render::renderPage(page, kNoImages, 1.0).pixelColor(100, 50), Qt::red, 2));
        page.grid.drawnAbove = true;
        QVERIFY(near(render::renderPage(page, kNoImages, 1.0).pixelColor(100, 50), Qt::black, 10));
    }

    void thumbnailGridIsSmoothed()
    {
        render::PaintOptions options;
        options.thumbnail = true;
        const QImage image = render::renderPage(gridPage(core::GridLevel{}), kNoImages, 0.3125, options);
        int darkest = 255;
        for (int x = 28; x <= 35; ++x) {
            darkest = std::min(darkest, image.pixelColor(x, 10).red());
        }
        QVERIFY2(darkest > 30 && darkest < 245,
                 qPrintable(QString::number(darkest))); // 0,7 px lissé, jamais net
    }

    void linesConnectorsAndSinglePointsAreDrawn()
    {
        core::Line line;
        line.points = {{10, 10}, {200, 10}};
        line.pen.width = 4;
        core::Connector connector;
        connector.firstEnd = {10, 50};
        connector.secondEnd = {200, 50};
        connector.pen.width = 4;
        core::Stroke dot;
        dot.kind = core::StrokeKind::Highlighter;
        dot.cap = core::PenCap::Round;
        dot.baseWidth = 20;
        dot.color = 0xFF0000FFu;
        dot.points = {{300, 300, 0.5f}};
        core::Stroke twin = dot;
        twin.points = {{400, 300, 0.5f}, {400, 300, 0.5f}};
        core::Page page;
        page.objects.push_back(line);
        page.objects.push_back(connector);
        page.objects.push_back(dot);
        page.objects.push_back(twin);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        QVERIFY(near(image.pixelColor(100, 10), Qt::black, 10));
        QVERIFY(near(image.pixelColor(100, 50), Qt::black, 10));
        QVERIFY(near(image.pixelColor(300, 300), Qt::blue, 10));
        QVERIFY(near(image.pixelColor(400, 300), Qt::blue, 10));
        QVERIFY(near(image.pixelColor(300, 315), Qt::white, 2));
    }

    void gridDotsAndDashesAreBroken()
    {
        for (const core::GridLine kind : {core::GridLine::Dotted, core::GridLine::Dashed}) {
            core::GridLevel level;
            level.line = kind;
            const QImage image = render::renderPage(gridPage(level), kNoImages, 1.0);
            const QRect column(100, 1, 1, 98);
            QVERIFY(countPixels(image, column, isDark) > 5);
            QVERIFY(countPixels(image, column, isLight) > 5);
        }
    }

    void openPathStaysOpen()
    {
        core::Shape s;
        s.kind = core::ShapeKind::Path;
        s.paths = {{{0, 0}, {100, 0}, {100, 100}}};
        s.open = true;
        s.pen.width = 4;
        s.brush = core::solidFill(0xFFFF0000u);
        core::PageObject o = s;
        o.common.transform = core::translation(50, 50);
        core::Page page;
        page.objects.push_back(o);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        QVERIFY(near(image.pixelColor(150, 100), Qt::black, 10));
        QVERIFY(near(image.pixelColor(100, 100), Qt::white, 2)); // pas de segment de fermeture
        QVERIFY(near(image.pixelColor(130, 70), Qt::white, 2));  // pas de remplissage
    }

    void mediaShowsItsPlaceholderOrAFrame()
    {
        core::Media m;
        m.bounds = {0, 0, 200, 100};
        core::PageObject o = m;
        o.common.transform = core::translation(100, 100);
        core::Page page;
        page.objects.push_back(o);
        const int grey = render::renderPage(page, kNoImages, 1.0).pixelColor(120, 120).red();
        QVERIFY2(grey > 205 && grey < 240, qPrintable(QString::number(grey)));
        QImage source(2, 2, QImage::Format_ARGB32);
        source.fill(Qt::red);
        render::DecodedImages images;
        images.insert("vignette", source);
        m.placeholder = "vignette";
        core::PageObject withPlaceholder = m;
        withPlaceholder.common.transform = core::translation(100, 100);
        page.objects = {withPlaceholder};
        QVERIFY(near(render::renderPage(page, images.lookup(), 1.0).pixelColor(120, 120), Qt::red, 2));
    }

    void diagonalAndAngledGrids()
    {
        core::GridLevel level;
        level.pattern = core::GridPattern::Diagonal45;
        const QImage diagonal = render::renderPage(gridPage(level), kNoImages, 1.0);
        QVERIFY(darkBetween(diagonal, 123, 127, 25)); // x = 100 + y
        QVERIFY(near(diagonal.pixelColor(100, 25), Qt::white, 0));
        level.pattern = core::GridPattern::Angle;
        level.angle = 60;
        const QImage angled = render::renderPage(gridPage(level), kNoImages, 1.0);
        QVERIFY(darkBetween(angled, 126, 132, 50)); // x = 100 + y × cot 60°
        QVERIFY(!darkBetween(angled, 145, 155, 50));
        QVERIFY(near(angled.pixelColor(100, 50), Qt::white, 0));
    }

    void gridPointsAndBigDots()
    {
        core::GridLevel level;
        level.line = core::GridLine::Points;
        const QImage points = render::renderPage(gridPage(level), kNoImages, 1.0);
        QVERIFY(darkBetween(points, 100, 101, 100));
        QVERIFY(near(points.pixelColor(99, 99), Qt::white, 0));
        QVERIFY(near(points.pixelColor(98, 100), Qt::white, 0));
        QVERIFY(near(points.pixelColor(100, 98), Qt::white, 0));
        level.line = core::GridLine::BigDots;
        const QImage dots = render::renderPage(gridPage(level), kNoImages, 1.0);
        QVERIFY(near(dots.pixelColor(99, 99), Qt::black, 10));
        QVERIFY(near(dots.pixelColor(101, 101), Qt::black, 10));
        QVERIFY(near(dots.pixelColor(102, 100), Qt::white, 0));
    }

    void diagonalAndRadialGradients()
    {
        core::Page page;
        page.background.fill.kind = core::FillKind::Gradient;
        page.background.fill.color = 0xFF000000u;
        page.background.fill.color2 = 0xFFFFFFFFu;
        const auto between = [](const QImage& image, int x, int y) {
            const int v = image.pixelColor(x, y).red();
            return v > 40 && v < 215;
        };
        page.background.fill.gradient = core::GradientKind::DiagonalDown;
        const QImage down = render::renderPage(page, kNoImages, 0.25);
        QVERIFY(down.pixelColor(2, 2).red() < 20);
        QVERIFY(down.pixelColor(253, 189).red() > 235);
        QVERIFY(between(down, 253, 2) && between(down, 2, 189));
        page.background.fill.gradient = core::GradientKind::DiagonalUp;
        const QImage up = render::renderPage(page, kNoImages, 0.25);
        QVERIFY(up.pixelColor(253, 2).red() < 20);
        QVERIFY(up.pixelColor(2, 189).red() > 235);
        QVERIFY(between(up, 2, 2) && between(up, 253, 189));
        page.background.fill.gradient = core::GradientKind::Radial;
        const QImage radial = render::renderPage(page, kNoImages, 0.25);
        QVERIFY(radial.pixelColor(128, 96).red() < 20);
        QVERIFY(radial.pixelColor(2, 2).red() > 235);
    }
};

QTEST_MAIN(TstRenderPainter)
#include "tst_render_painter.moc"
