#include <actcraft/core/document.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/render/painternode.h>

#include <QBuffer>
#include <QImageReader>
#include <QPainter>
#include <QTest>
#include <QThread>
#include <QtConcurrent/QtConcurrentRun>

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
        const QImage image = QtConcurrent::run([&page, &worker] {
                                 worker = QThread::currentThread();
                                 return render::renderPage(page, kNoImages, 1.0);
                             }).result();
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
};

QTEST_MAIN(TstRenderPainter)
#include "tst_render_painter.moc"
