#include "support/renderhost.h"

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/imagestore.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/render/paintedrenderer.h>

#include <QBuffer>
#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QSignalSpy>
#include <QTest>

using namespace actcraft;

namespace {

core::PageObject pathSquare(double size, core::Argb fill)
{
    core::Shape s;
    s.kind = core::ShapeKind::Path;
    s.paths = {{{0, 0}, {size, 0}, {size, size}, {0, size}}};
    s.pen.style = core::PenStyle::None;
    s.brush = core::solidFill(fill);
    return core::PageObject(s);
}

core::PageObject redText()
{
    core::Text t;
    t.bounds = {0, 0, 300, 60};
    t.html = "<p style=\"color:#ff0000; font-size:40px\">XXXX</p>";
    core::PageObject o = t;
    o.common.transform = core::translation(100, 100);
    return o;
}

// Texte rouge suivi d'une image sur un hôte UNC inexistant (nom tiré au hasard : aucun cache de résolution).
core::PageObject hostileText()
{
    const std::string host = "actcraft-sonde-" + std::to_string(QRandomGenerator::global()->generate64());
    core::Text t;
    t.bounds = {0, 0, 300, 120};
    t.html = "<p style=\"color:#ff0000; font-size:40px\">XXXX<img src=\"\\\\" + host +
             "\\x\\x.png\" width=\"60\" height=\"60\"></p>";
    core::PageObject o = t;
    o.common.transform = core::translation(100, 100);
    return o;
}

const render::ImageLookup kNoImages = [](const std::string&) { return QImage(); };

QByteArray pngOf(const QImage& image)
{
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return png;
}

// Pixels rouges du rectangle `logical` (coordonnées logiques) d'une capture à `ratio` pixels par unité
// logique (grabWindow rend une image en pixels du périphérique sans poser sa densité).
int redPixels(const QImage& image, const QRect& logical, double ratio = 1)
{
    int red = 0;
    for (int y = static_cast<int>(logical.top() * ratio); y < static_cast<int>(logical.bottom() * ratio);
         ++y) {
        for (int x = static_cast<int>(logical.left() * ratio); x < static_cast<int>(logical.right() * ratio);
             ++x) {
            const QColor c = image.pixelColor(x, y);
            red += c.red() > 200 && c.green() < 80 ? 1 : 0;
        }
    }
    return red;
}

// Élément de test : un seul objet par le moteur peint ; le nœud est construit une fois puis gardé.
class PaintedHostItem : public QQuickItem
{
public:
    PaintedHostItem() { setFlag(ItemHasContents); }

    core::PageObject object;
    const render::ImageStore* images = nullptr;

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData*) override
    {
        if (old) {
            return old;
        }
        const bool software = window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software;
        QSGNode* root = software ? new QSGTransformNode : new QSGNode;
        render::RenderContext context;
        context.window = window();
        context.software = software;
        context.images = images;
        if (QSGNode* node = render::PaintedRenderer().createNode(object, context)) {
            root->appendChildNode(node);
        }
        return root;
    }
};

} // namespace

// Moteur peint, fond, grille, transformations dans la page fixe (rendu logiciel des tests).
class TstRenderPainted : public QObject
{
    Q_OBJECT

private:
    struct Fixture
    {
        QQuickWindow window;
        render::FixedPageItem* page = nullptr;
    };

    static bool open(Fixture& f, const core::Page& page)
    {
        f.window.setColor(QColor(0x3c, 0x3f, 0x45));
        f.window.resize(1024, 768);
        f.page = new render::FixedPageItem;
        f.page->setSize(QSizeF(1024, 768));
        f.page->setParentItem(f.window.contentItem());
        f.page->setPage(page);
        f.window.show();
        return QTest::qWaitForWindowExposed(&f.window) &&
               QTest::qWaitFor([&f] { return f.page->isReady(); }, 5000);
    }

    static QImage grab(Fixture& f)
    {
        QSignalSpy swapped(&f.window, &QQuickWindow::frameSwapped);
        f.page->update();
        swapped.wait(2000);
        return f.window.grabWindow();
    }

private slots:
    void usesPaintedRendererChoosesTheEngine()
    {
        QVERIFY(render::usesPaintedRenderer(redText()));
        QVERIFY(render::usesPaintedRenderer(core::PageObject(core::Line{})));
        QVERIFY(render::usesPaintedRenderer(core::PageObject(core::Media{})));
        QVERIFY(render::usesPaintedRenderer(core::PageObject(core::Connector{})));
        QVERIFY(render::usesPaintedRenderer(pathSquare(10, 0xFF000000u)));
        core::Stroke highlighter;
        highlighter.kind = core::StrokeKind::Highlighter;
        QVERIFY(render::usesPaintedRenderer(core::PageObject(highlighter)));
        core::Stroke round;
        round.cap = core::PenCap::Round;
        QVERIFY(render::usesPaintedRenderer(core::PageObject(round)));
        QVERIFY(!render::usesPaintedRenderer(core::PageObject(core::Stroke{})));
        QVERIFY(!render::usesPaintedRenderer(core::PageObject(core::Shape{})));
        QVERIFY(!render::usesPaintedRenderer(core::PageObject(core::ImageObject{})));
        QVERIFY(!render::usesPaintedRenderer(core::PageObject(core::Group{})));
    }

    void rasterizeLocalCoversTheObjectAtScale()
    {
        QRectF rect;
        const QImage image = render::rasterizeLocal(redText(), kNoImages, 2.0, &rect);
        QCOMPARE(rect, QRectF(0, 0, 300, 60));
        QVERIFY(image.width() >= 600 && image.height() >= 120);
        bool red = false;
        for (int y = 0; y < image.height() && !red; ++y) {
            for (int x = 0; x < image.width() && !red; ++x) {
                red = image.pixelColor(x, y).red() > 200 && image.pixelColor(x, y).alpha() > 200 &&
                      image.pixelColor(x, y).green() < 80;
            }
        }
        QVERIFY(red);
    }

    void textIsDrawnInTheScene()
    {
        core::Page page;
        page.objects.push_back(redText());
        Fixture f;
        QVERIFY(open(f, page));
        const QImage image = grab(f);
        const double ratio = f.window.devicePixelRatio();
        const int red = redPixels(image, QRect(QPoint(100, 100), QPoint(400, 160)), ratio);
        QVERIFY2(red > 200, qPrintable(QString::number(red)));
    }

    void transformedAndTranslucentObjects()
    {
        core::Page page;
        core::PageObject rotated = pathSquare(40, 0xFF0000FFu);
        rotated.common.transform = core::compose(core::rotation(90), core::translation(200, 200));
        page.objects.push_back(rotated);
        core::Shape legacy; // moteur du prototype, sous un nœud de transformation
        legacy.kind = core::ShapeKind::Rectangle;
        legacy.bounds = {0, 0, 40, 40};
        legacy.fill = 0xFF00FF00u;
        legacy.outlineWidth = 0;
        core::PageObject moved = legacy;
        moved.common.transform = core::translation(400, 100);
        page.objects.push_back(moved);
        core::PageObject half = pathSquare(40, 0xFF000000u);
        half.common.transform = core::translation(600, 100);
        half.common.translucency = 128;
        page.objects.push_back(half);
        core::PageObject hidden = pathSquare(40, 0xFF000000u);
        hidden.common.transform = core::translation(800, 100);
        hidden.common.visible = false;
        page.objects.push_back(hidden);
        Fixture f;
        QVERIFY(open(f, page));
        const QImage image = grab(f);
        QVERIFY(colorNear(image.pixelColor(190, 210), QColor(Qt::blue), 3));
        QVERIFY(colorNear(image.pixelColor(210, 210), QColor(Qt::white), 3));
        QVERIFY(colorNear(image.pixelColor(420, 120), QColor(Qt::green), 3));
        const int grey = image.pixelColor(620, 120).red();
        QVERIFY2(grey > 110 && grey < 145, qPrintable(QString::number(grey)));
        QVERIFY(colorNear(image.pixelColor(820, 120), QColor(Qt::white), 1));
    }

    void backgroundAndGrid()
    {
        core::Page page;
        page.background.fill.kind = core::FillKind::Gradient;
        page.background.fill.color = 0xFF000000u;
        page.background.fill.color2 = 0xFFFFFFFFu;
        page.background.fill.gradient = core::GradientKind::Vertical;
        Fixture f;
        QVERIFY(open(f, page));
        const QImage image = grab(f);
        QVERIFY(image.pixelColor(500, 3).red() < 30);
        QVERIFY(image.pixelColor(500, 764).red() > 225);
    }

    void gridAtTopStaysAboveObjects()
    {
        core::Page page;
        core::PageObject red = pathSquare(300, 0xFFFF0000u);
        page.objects.push_back(red);
        page.grid.visible = true;
        page.grid.drawnAbove = true;
        core::GridLevel level;
        level.spacingX = 100;
        level.spacingY = 100;
        page.grid.levels.push_back(level);
        Fixture f;
        QVERIFY(open(f, page));
        const QImage image = grab(f);
        QVERIFY(colorNear(image.pixelColor(100, 50), QColor(Qt::black),
                          20)); // trait de grille sur le carré rouge
        QVERIFY(colorNear(image.pixelColor(150, 50), QColor(Qt::red), 3));
    }

    void sceneMatchesThePainter()
    {
        core::Page page;
        page.background.fill.kind = core::FillKind::Gradient;
        page.background.fill.color = 0xFFC8DCFFu;
        page.background.fill.color2 = 0xFFFFFFFFu;
        core::Shape rect;
        rect.kind = core::ShapeKind::Rectangle;
        rect.bounds = {50, 50, 200, 120};
        rect.fill = 0xFF336699u;
        rect.outlineWidth = 3;
        page.objects.push_back(rect);
        core::PageObject square = pathSquare(150, 0xFF00AA00u);
        square.common.transform = core::compose(core::rotation(30), core::translation(500, 120));
        page.objects.push_back(square);
        page.objects.push_back(redText());
        core::Line line;
        line.points = {{600, 500}, {900, 650}};
        line.pen.width = 6;
        line.pen.style = core::PenStyle::Dash;
        page.objects.push_back(line);
        core::Stroke pen;
        pen.baseWidth = 8;
        pen.points = {{100, 600, 0.5f}, {300, 650, 0.5f}, {400, 600, 0.5f}};
        page.objects.push_back(pen);
        core::Stroke marker;
        marker.kind = core::StrokeKind::Highlighter;
        marker.cap = core::PenCap::Round;
        marker.baseWidth = 30;
        marker.color = 0x80FFFF00u;
        marker.points = {{100, 400, 0.5f}, {500, 420, 0.5f}};
        core::PageObject top = marker;
        top.common.layer = core::Layer::Top;
        page.objects.push_back(top);
        Fixture f;
        QVERIFY(open(f, page));
        const QImage scene = grab(f).convertToFormat(QImage::Format_RGB32);
        const QImage painted = render::renderPage(page, kNoImages, 1.0).convertToFormat(QImage::Format_RGB32);
        QCOMPARE(scene.size(), painted.size());
        double total = 0;
        for (int y = 0; y < scene.height(); ++y) {
            for (int x = 0; x < scene.width(); ++x) {
                const QColor a = scene.pixelColor(x, y);
                const QColor b = painted.pixelColor(x, y);
                total += qAbs(a.red() - b.red()) + qAbs(a.green() - b.green()) + qAbs(a.blue() - b.blue());
            }
        }
        const double mean = total / (3.0 * scene.width() * scene.height());
        QVERIFY2(mean < 2.0, qPrintable(QString::number(mean)));
    }

    void rasterScaleNeverRebuildsInSoftware()
    {
        core::Page page;
        page.objects.push_back(redText()); // objet peint : en RHI, une nouvelle échelle le rastériserait
        Fixture f;
        QVERIFY(open(f, page));
        grab(f);
        const int before = f.page->rebuildCount();
        f.page->setRasterScale(2.0);
        QCOMPARE(f.page->rasterScale(), 2.0);
        grab(f);
        QCOMPARE(f.page->rebuildCount(), before); // logiciel : nœuds vectoriels, rien à rastériser
        f.page->setRasterScale(2.0);              // même échelle : rien
        grab(f);
        QCOMPARE(f.page->rebuildCount(), before);
        // Rouge vu par mutation : retirer le test « !software » de setRasterScale → rebuildCount + 1.
    }

    // Le nœud peint copie à sa création les images dont il a besoin : un nœud gardé pendant que le magasin
    // passe à une autre page dessine toujours les siennes.
    void paintedNodeKeepsItsImages()
    {
        core::Document document;
        QImage red(8, 8, QImage::Format_ARGB32);
        red.fill(Qt::red);
        core::Media media;
        media.bounds = {0, 0, 100, 100};
        media.placeholder = document.addResource(pngOf(red), "png");
        core::Page page;
        page.objects.push_back(core::PageObject(media));
        render::ImageStore store;
        store.generate(page, &document);
        QTRY_VERIFY_WITH_TIMEOUT(store.isReady(), 5000);
        QVERIFY(!store.resourceImage(media.placeholder).isNull());
        QQuickWindow window; // détruite avant le magasin
        window.setColor(Qt::white);
        window.resize(200, 200);
        auto* host = new PaintedHostItem;
        host->object = page.objects.front();
        host->images = &store;
        host->setSize(QSizeF(200, 200));
        host->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY(colorNear(window.grabWindow().pixelColor(50, 50), QColor(Qt::red), 3));
        store.generate(core::Page{}); // page suivante : la ressource quitte le magasin
        QTRY_VERIFY_WITH_TIMEOUT(store.isReady(), 5000);
        QVERIFY(store.resourceImage(media.placeholder).isNull());
        host->update();
        QVERIFY(colorNear(window.grabWindow().pixelColor(50, 50), QColor(Qt::red), 3));
    }

    // Le texte d'un document ne charge aucune ressource, à la densité de l'écran (ctest
    // tst_render_painted_density : QT_SCALE_FACTOR=2). Une image sur un hôte UNC inexistant coûterait une
    // résolution réseau (environ 1,3 s) au premier rendu de la scène.
    void htmlImagesNeverBlockTheScene()
    {
        core::Page warm;
        warm.objects.push_back(redText());
        Fixture f;
        QVERIFY(open(f, warm));
        grab(f); // polices chargées
        const QByteArray factor = qgetenv("QT_SCALE_FACTOR");
        if (!factor.isEmpty()) {
            QCOMPARE(f.window.devicePixelRatio(), factor.toDouble());
        }
        // Page masquée pendant sa construction : le premier rendu du texte est celui que l'on chronomètre.
        f.page->setVisible(false);
        grab(f);
        core::Page hostile;
        hostile.objects.push_back(hostileText());
        f.page->setPage(hostile);
        QVERIFY(QTest::qWaitFor([&f] { return f.page->isReady(); }, 5000));
        QElapsedTimer timer;
        timer.start();
        f.page->setVisible(true);
        const QImage image = f.window.grabWindow();
        const qint64 sceneMs = timer.elapsed();
        QVERIFY2(sceneMs < 100, qPrintable(QString::number(sceneMs)));
        const double ratio = f.window.devicePixelRatio();
        const int red = redPixels(image, QRect(QPoint(100, 100), QPoint(400, 160)), ratio);
        QVERIFY2(red > 200, qPrintable(QStringLiteral("%1 rouges, %2 x %3 à %4")
                                           .arg(red)
                                           .arg(image.width())
                                           .arg(image.height())
                                           .arg(ratio)));
        // Chemin RHI : même peintre, rastérisé à l'échelle de l'écran.
        timer.restart();
        QRectF rect;
        const QImage raster = render::rasterizeLocal(hostileText(), kNoImages, 2.0, &rect);
        const qint64 rasterMs = timer.elapsed();
        QVERIFY2(rasterMs < 100, qPrintable(QString::number(rasterMs)));
        QVERIFY(redPixels(raster, raster.rect()) > 200);
    }
};

QTEST_MAIN(TstRenderPainted)
#include "tst_render_painted.moc"
