#include "support/renderhost.h"

#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/gridoverlayitem.h>
#include <actcraft/render/imagestore.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/render/paintedrenderer.h>

#include <QBuffer>
#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QSGImageNode>
#include <QSGTexture>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include <algorithm>
#include <functional>
#include <memory>

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

// Texte rouge suivi de deux images : un fichier local bleu (`bluePng`) et un hôte UNC inexistant (nom tiré au
// hasard : aucun cache de résolution). Le peintre n'en charge aucune.
core::PageObject hostileText(const QString& bluePng)
{
    const std::string host = "actcraft-sonde-" + std::to_string(QRandomGenerator::global()->generate64());
    core::Text t;
    t.bounds = {0, 0, 400, 120};
    t.html = "<p style=\"color:#ff0000; font-size:40px\">XXXX<img src=\"" +
             QUrl::fromLocalFile(bluePng).toString().toStdString() +
             "\" width=\"60\" height=\"60\"><img src=\"\\\\" + host +
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

// Pixels du rectangle `logical` (coordonnées logiques) d'une capture à `ratio` pixels par unité logique
// (grabWindow rend une image en pixels du périphérique sans poser sa densité) qui vérifient `match`.
int countPixels(const QImage& image, const QRect& logical, double ratio,
                const std::function<bool(QColor)>& match)
{
    int count = 0;
    const int top = std::max(0, static_cast<int>(logical.top() * ratio));
    const int bottom = std::min(image.height(), static_cast<int>(logical.bottom() * ratio));
    const int left = std::max(0, static_cast<int>(logical.left() * ratio));
    const int right = std::min(image.width(), static_cast<int>(logical.right() * ratio));
    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            count += match(image.pixelColor(x, y)) ? 1 : 0;
        }
    }
    return count;
}

bool isRed(QColor c)
{
    return c.red() > 200 && c.green() < 80 && c.blue() < 80;
}

bool isBlue(QColor c)
{
    return c.blue() > 200 && c.red() < 80 && c.green() < 80;
}

int redPixels(const QImage& image, const QRect& logical, double ratio = 1)
{
    return countPixels(image, logical, ratio, isRed);
}

// Écart moyen par canal entre deux images de même taille sur `zone`.
double meanDifference(const QImage& a, const QImage& b, const QRect& zone)
{
    const QRect r = zone.intersected(a.rect());
    if (r.isEmpty()) {
        return 0;
    }
    double total = 0;
    for (int y = r.top(); y <= r.bottom(); ++y) {
        for (int x = r.left(); x <= r.right(); ++x) {
            const QColor p = a.pixelColor(x, y);
            const QColor q = b.pixelColor(x, y);
            total += qAbs(p.red() - q.red()) + qAbs(p.green() - q.green()) + qAbs(p.blue() - q.blue());
        }
    }
    return total / (3.0 * r.width() * r.height());
}

// Élément de test : un nœud construit une fois par `build`, puis gardé.
class NodeHostItem : public QQuickItem
{
public:
    NodeHostItem() { setFlag(ItemHasContents); }

    std::function<QSGNode*(const render::RenderContext&)> build;
    const render::ImageStore* images = nullptr;
    bool rasterized = false; // chemin RHI (textures), même en rendu logiciel

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
        context.software = software && !rasterized;
        context.images = images;
        if (QSGNode* node = build(context)) {
            root->appendChildNode(node);
        }
        return root;
    }
};

struct HostFixture
{
    QQuickWindow window;
    NodeHostItem* host = nullptr;
};

bool openHost(HostFixture& f, std::function<QSGNode*(const render::RenderContext&)> build,
              const render::ImageStore* images, bool rasterized)
{
    f.window.setColor(QColor(0x3c, 0x3f, 0x45));
    f.window.resize(1024, 768);
    f.host = new NodeHostItem;
    f.host->build = std::move(build);
    f.host->images = images;
    f.host->rasterized = rasterized;
    f.host->setSize(QSizeF(1024, 768));
    f.host->setParentItem(f.window.contentItem());
    f.window.show();
    return QTest::qWaitForWindowExposed(&f.window);
}

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

    static bool open(Fixture& f, const core::Page& page, bool rasterized = false)
    {
        f.window.setColor(QColor(0x3c, 0x3f, 0x45));
        f.window.resize(1024, 768);
        f.page = new render::FixedPageItem;
        f.page->setForceRasterized(rasterized);
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
        // Amendé par L3-T11 : la géométrie sait faire les bouts ronds, le stylo opaque y revient (seul un
        // trait translucide reste au moteur peint, voir tst_render_strokeshape).
        QVERIFY(!render::usesPaintedRenderer(core::PageObject(round)));
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
        QVERIFY(redPixels(image, image.rect()) > 0);
    }

    // Le rectangle rastérisé suit la largeur réellement dessinée : surligneur à largeur constante (pas celle
    // de la pression), bout carré d'une diagonale.
    void rasterizeLocalCoversTheDrawnWidth()
    {
        core::Stroke marker;
        marker.kind = core::StrokeKind::Highlighter;
        marker.cap = core::PenCap::Round;
        marker.baseWidth = 30;
        marker.points = {{100, 400, 0.1f}, {500, 400, 0.1f}};
        QRectF rect;
        render::rasterizeLocal(core::PageObject(marker), kNoImages, 1.0, &rect);
        QVERIFY2(rect.contains(QRectF(85, 385, 430, 30)), qPrintable(QStringLiteral("%1 %2 %3 %4")
                                                                         .arg(rect.x())
                                                                         .arg(rect.y())
                                                                         .arg(rect.width())
                                                                         .arg(rect.height())));
        core::Line diagonal;
        diagonal.points = {{0, 0}, {100, 100}};
        diagonal.pen.width = 20;
        diagonal.pen.cap = core::PenCap::Square;
        render::rasterizeLocal(core::PageObject(diagonal), kNoImages, 1.0, &rect);
        QVERIFY(rect.contains(QPointF(-14.2, 0)) && rect.contains(QPointF(0, -14.2)));
        QVERIFY(rect.contains(QPointF(114.2, 100)) && rect.contains(QPointF(100, 114.2)));
    }

    void textIsDrawnInTheScene()
    {
        core::Page page;
        page.objects.push_back(redText());
        Fixture f;
        QVERIFY(open(f, page));
        const QImage image = grab(f);
        const int red = redPixels(image, QRect(QPoint(100, 100), QPoint(400, 160)));
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

    // Un fond qui ne couvre pas la page (dégradé ou couleur translucide, image à transparence sur fond «
    // aucun ») laisse voir le blanc de la page, comme le peintre, pas ce qui est derrière l'élément.
    void backgroundLiesOnWhitePage()
    {
        core::Document document;
        QImage halfGreen(8, 8, QImage::Format_ARGB32);
        halfGreen.fill(Qt::transparent);
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 4; ++x) {
                halfGreen.setPixelColor(x, y, QColor(Qt::green));
            }
        }
        const std::string key = document.addResource(pngOf(halfGreen), "png");
        std::vector<core::Page> pages(3);
        pages[0].background.fill.kind = core::FillKind::Gradient;
        pages[0].background.fill.color = 0x80FF0000u;
        pages[0].background.fill.color2 = 0x400000FFu;
        pages[1].background.fill = core::solidFill(0x80FF0000u);
        pages[2].background.fill.kind = core::FillKind::None;
        pages[2].background.image = key;
        for (std::size_t i = 0; i < pages.size(); ++i) {
            render::ImageStore store;
            store.generate(pages[i], &document);
            QTRY_VERIFY_WITH_TIMEOUT(store.isReady(), 5000);
            const QImage painted = render::renderPage(
                pages[i], [&store](const std::string& k) { return store.resourceImage(k); }, 1.0);
            for (const bool rasterized : {false, true}) {
                HostFixture f;
                const core::Page page = pages[i];
                QVERIFY(openHost(
                    f,
                    [page](const render::RenderContext& c) { return render::createBackgroundNode(page, c); },
                    &store, rasterized));
                const QImage scene = f.window.grabWindow();
                for (const QPoint p : {QPoint(60, 60), QPoint(900, 600), QPoint(512, 700)}) {
                    QVERIFY2(colorNear(scene.pixelColor(p), painted.pixelColor(p), 3),
                             qPrintable(QStringLiteral("fond %1, %2 : scène %3, peintre %4")
                                            .arg(i)
                                            .arg(rasterized ? QStringLiteral("rastérisé")
                                                            : QStringLiteral("vectoriel"))
                                            .arg(scene.pixelColor(p).name(), painted.pixelColor(p).name())));
                }
            }
        }
    }

    // Changement de règle voulu (L4-T7) : la grille du dessus n'est plus dans la scène de la page fixe (celle
    // que le cache photographie) ; la surcouche GridOverlayItem, posée au-dessus, la dessine sur les objets.
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
        QVERIFY(
            colorNear(grab(f).pixelColor(100, 50), QColor(Qt::red), 3)); // page fixe seule : pas de grille
        auto* overlay = new render::GridOverlayItem;
        overlay->setSize(QSizeF(1024, 768));
        overlay->setParentItem(f.window.contentItem());
        overlay->setFixedPage(f.page);
        const QImage image = grab(f);
        QVERIFY(colorNear(image.pixelColor(100, 50), QColor(Qt::black),
                          20)); // trait de grille sur le carré rouge
        QVERIFY(colorNear(image.pixelColor(150, 50), QColor(Qt::red), 3));
    }

    // Image illisible : la scène dessine le repère du peintre, comme la miniature.
    void unreadableImageShowsTheMarker()
    {
        core::Page page;
        core::ImageObject unreadable;
        unreadable.bounds = {100, 100, 200, 150};
        unreadable.resource = "ressource-absente";
        page.objects.push_back(core::PageObject(unreadable));
        Fixture f;
        QVERIFY(open(f, page));
        const QImage scene = grab(f);
        const QImage painted = render::renderPage(page, kNoImages, 1.0);
        QVERIFY(!colorNear(painted.pixelColor(200, 175), QColor(Qt::white), 3));
        QVERIFY2(colorNear(scene.pixelColor(200, 175), painted.pixelColor(200, 175), 3),
                 qPrintable(scene.pixelColor(200, 175).name() + QLatin1Char(' ') +
                            painted.pixelColor(200, 175).name()));
    }

    // Tous les types, une grille, une rotation et un miroir : la scène rend l'image du peintre, objet par
    // objet (chaque objet a ses pixels) et en moyenne.
    void sceneMatchesThePainter()
    {
        core::Page page;
        page.background.fill.kind = core::FillKind::Gradient;
        page.background.fill.color = 0xFFC8DCFFu;
        page.background.fill.color2 = 0xFFFFFFFFu;
        page.grid.visible = true;
        core::GridLevel level;
        level.spacingX = 64;
        level.spacingY = 64;
        level.color = 0xFFB0B0B0u;
        page.grid.levels.push_back(level);
        core::Shape rect;
        rect.kind = core::ShapeKind::Rectangle;
        rect.bounds = {50, 50, 200, 120};
        rect.fill = 0xFF336699u;
        rect.outlineWidth = 3;
        page.objects.push_back(rect);
        core::PageObject square = pathSquare(150, 0xFF00AA00u);
        square.common.transform = core::compose(core::rotation(30), core::translation(500, 120));
        page.objects.push_back(square);
        core::Shape triangle;
        triangle.kind = core::ShapeKind::Path;
        triangle.paths = {{{0, 0}, {80, 0}, {0, 60}}};
        triangle.pen.width = 2;
        triangle.brush = core::solidFill(0xFFAA00AAu);
        core::PageObject mirrored = triangle;
        mirrored.common.transform.m11 = -1; // miroir horizontal
        mirrored.common.transform.dx = 980;
        mirrored.common.transform.dy = 660;
        page.objects.push_back(mirrored);
        page.objects.push_back(redText());
        core::Line line;
        line.points = {{600, 500}, {900, 650}};
        line.pen.width = 6;
        line.pen.style = core::PenStyle::Dash;
        page.objects.push_back(line);
        core::Connector connector;
        connector.firstEnd = {700, 80};
        connector.secondEnd = {950, 200};
        connector.pen.width = 4;
        page.objects.push_back(connector);
        core::Media media;
        media.bounds = {750, 300, 150, 100};
        page.objects.push_back(media);
        core::ImageObject picture;
        picture.bounds = {300, 250, 120, 90};
        picture.sourceWidth = 120;
        picture.sourceHeight = 90;
        picture.pattern = core::ImagePattern::Checker;
        page.objects.push_back(picture);
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
        for (std::size_t i = 0; i < page.objects.size(); ++i) {
            const core::RectF b = core::boundsOnPage(page.objects[i]);
            const QRect zone = QRectF(b.x, b.y, b.width, b.height).toAlignedRect();
            QVERIFY2(!zone.isEmpty(), qPrintable(QString::number(i)));
            const double zoneMean = meanDifference(scene, painted, zone);
            QVERIFY2(zoneMean < 1.0, qPrintable(QStringLiteral("objet %1 : %2").arg(i).arg(zoneMean)));
        }
        const double mean = meanDifference(scene, painted, scene.rect());
        QVERIFY2(mean < 0.05, qPrintable(QString::number(mean)));
    }

    // L2-C9, constat 8 (F-574) : motif à fond opaque. ActivInspire peint la couleur de fond sous le motif,
    // avec l'alpha de la couleur du contour ; en mode transparent, rien sous le motif. Écran (moteur peint,
    // direct et en texture) et miniature (peintre partagé) : mêmes pixels.
    void opaquePatternBackgroundIsDrawn()
    {
        core::Shape opaque;
        opaque.kind = core::ShapeKind::Path;
        opaque.paths = {{{100, 100}, {300, 100}, {300, 300}, {100, 300}}};
        opaque.pen.style = core::PenStyle::None;
        opaque.pen.color = 0xFF000000u;
        opaque.brush.kind = core::FillKind::Pattern;
        opaque.brush.color = 0xFF000000u;
        opaque.brush.pattern = 5; // Qt::Dense4Pattern : un pixel sur deux
        opaque.brush.opaqueBackground = true;
        opaque.brush.backgroundColor = 0xFFFF0000u;
        core::Shape transparent = opaque; // mode transparent : fond ignoré
        transparent.paths = {{{400, 100}, {600, 100}, {600, 300}, {400, 300}}};
        transparent.brush.opaqueBackground = false;
        core::Shape clearPen = opaque; // contour d'alpha nul : fond d'alpha nul
        clearPen.paths = {{{700, 100}, {900, 100}, {900, 300}, {700, 300}}};
        clearPen.pen.color = 0x00000000u;
        core::Page page;
        page.objects = {core::PageObject(opaque), core::PageObject(transparent), core::PageObject(clearPen)};
        const QImage painted = render::renderPage(page, kNoImages, 1.0).convertToFormat(QImage::Format_RGB32);
        const QRect inside(110, 110, 180, 180);
        const int area = inside.width() * inside.height();
        QVERIFY2(redPixels(painted, inside) > area / 3,
                 qPrintable(QString::number(redPixels(painted, inside))));
        QCOMPARE(redPixels(painted, inside.translated(300, 0)), 0);
        QCOMPARE(redPixels(painted, inside.translated(600, 0)), 0);
        for (const bool rasterized : {false, true}) {
            Fixture f;
            QVERIFY(open(f, page, rasterized));
            const QImage scene = grab(f).convertToFormat(QImage::Format_RGB32);
            QCOMPARE(scene.size(), painted.size());
            for (const int dx : {0, 300, 600}) {
                const QRect zone = inside.translated(dx, 0);
                QCOMPARE(redPixels(scene, zone), redPixels(painted, zone));
                const double mean = meanDifference(scene, painted, zone);
                QVERIFY2(mean < 1.0,
                         qPrintable(QStringLiteral("%1 %2 : %3").arg(rasterized).arg(dx).arg(mean)));
            }
        }
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
        QCOMPARE(f.page->rasterizationCount(), 0);
    }

    // Chemin RHI (forcé en rendu logiciel) : une nouvelle échelle ne refait que les nœuds rastérisés (fond
    // peint, grille, objets peints), en place ; les nœuds du prototype restent ceux de la construction.
    void rasterScaleRedrawsOnlyRasterizedNodes()
    {
        core::Page page;
        page.background.fill.kind = core::FillKind::Gradient;
        page.background.fill.color = 0xFFC8DCFFu;
        page.grid.visible = true;
        page.grid.levels.push_back(core::GridLevel{});
        for (int i = 0; i < 3; ++i) {
            core::Stroke stroke; // moteur du prototype
            stroke.points = {{50, 300.0 + 20 * i, 0.5f}, {400, 300.0 + 20 * i, 0.5f}};
            page.objects.push_back(stroke);
        }
        page.objects.push_back(redText());
        page.objects.push_back(pathSquare(40, 0xFF0000FFu));
        Fixture f;
        QVERIFY(open(f, page, true));
        grab(f);
        QCOMPARE(f.page->rasterizationCount(), 4); // fond, grille, texte, forme à chemins
        const int rebuilds = f.page->rebuildCount();
        f.page->setRasterScale(2.0);
        const QImage image = grab(f);
        QCOMPARE(f.page->rebuildCount(), rebuilds);
        QCOMPARE(f.page->rasterizationCount(), 8);
        QVERIFY(redPixels(image, QRect(QPoint(100, 100), QPoint(400, 160))) > 200); // texte remis à sa place
        QVERIFY(colorNear(image.pixelColor(20, 20), QColor(Qt::blue), 3));
        QVERIFY(colorNear(image.pixelColor(200, 340), QColor(Qt::black), 40)); // trait du prototype gardé
    }

    // Page du banc (objets du prototype, fond uni, sans grille) : un zoom ne reconstruit ni ne rastérise
    // rien, même sur le chemin RHI.
    void benchPageIsNeverRebuiltOnZoom()
    {
        Fixture f;
        QVERIFY(open(f, core::Page{}, true));
        f.page->loadGeneratedPage();
        QVERIFY(QTest::qWaitFor([&f] { return f.page->isReady(); }, 20000));
        grab(f);
        const int rebuilds = f.page->rebuildCount();
        f.page->setRasterScale(1.5);
        grab(f);
        f.page->setRasterScale(3.0);
        grab(f);
        QCOMPARE(f.page->rebuildCount(), rebuilds);
        QCOMPARE(f.page->rasterizationCount(), 0);
    }

    // Une texture rastérisée reste sous render::kMaxRasterPixels, quelle que soit la page ou l'échelle.
    void rasterTexturesAreBounded()
    {
        QQuickWindow window;
        window.resize(200, 200);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.grabWindow(); // graphe de scène prêt
        core::Page page;
        page.width = 3000;
        page.height = 3000;
        page.background.fill.kind = core::FillKind::Gradient;
        render::RenderContext context;
        context.window = &window;
        context.rasterScale = 2.0; // 36 Mpx sans borne
        std::unique_ptr<QSGNode> background(render::createBackgroundNode(page, context));
        QSGImageNode* texture = nullptr;
        for (QSGNode* child = background->firstChild(); child && !texture; child = child->nextSibling()) {
            texture = dynamic_cast<QSGImageNode*>(child);
        }
        QVERIFY(texture);
        const QSize size = texture->texture()->textureSize();
        const double pixels = static_cast<double>(size.width()) * size.height();
        QVERIFY2(pixels <= render::kMaxRasterPixels && pixels > render::kMaxRasterPixels / 2,
                 qPrintable(QString::number(pixels)));
        QCOMPARE(texture->rect(), QRectF(0, 0, 3000, 3000));
        QRectF rect;
        const QImage image = render::rasterizeLocal(pathSquare(3000, 0xFF000000u), kNoImages, 2.0, &rect);
        const double objectPixels = static_cast<double>(image.width()) * image.height();
        QVERIFY2(objectPixels <= render::kMaxRasterPixels && objectPixels > render::kMaxRasterPixels / 2,
                 qPrintable(QString::number(objectPixels)));
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
        HostFixture f; // détruite avant le magasin
        const core::PageObject object = page.objects.front();
        QVERIFY(openHost(
            f,
            [object](const render::RenderContext& c) {
                return render::PaintedRenderer().createNode(object, c);
            },
            &store, false));
        QVERIFY(colorNear(f.window.grabWindow().pixelColor(50, 50), QColor(Qt::red), 3));
        store.generate(core::Page{}); // page suivante : la ressource quitte le magasin
        QTRY_VERIFY_WITH_TIMEOUT(store.isReady(), 5000);
        QVERIFY(store.resourceImage(media.placeholder).isNull());
        // Nœud gardé mais redessiné : l'élément se déplace (le rendu logiciel ne repeint que ce qui change).
        f.host->setPosition(QPointF(10, 10));
        QVERIFY(colorNear(f.window.grabWindow().pixelColor(60, 60), QColor(Qt::red), 3));
    }

    // Le texte d'un document ne charge aucune ressource, à la densité de l'écran (ctest
    // tst_render_painted_density : QT_SCREEN_SCALE_FACTORS=2). Preuve déterministe : l'image locale bleue du
    // HTML n'est jamais dessinée ; le temps (hôte UNC inexistant, environ 1,3 s s'il était résolu) reste une
    // garde large.
    void htmlImagesNeverBlockTheScene()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString bluePng = dir.filePath(QStringLiteral("bleu.png"));
        QImage blue(60, 60, QImage::Format_ARGB32);
        blue.fill(Qt::blue);
        QVERIFY(blue.save(bluePng, "PNG"));
        core::Page warm;
        warm.objects.push_back(redText());
        Fixture f;
        QVERIFY(open(f, warm));
        grab(f); // polices chargées
        const QByteArray factor = qgetenv("QT_SCREEN_SCALE_FACTORS");
        if (!factor.isEmpty()) {
            QCOMPARE(f.window.devicePixelRatio(), factor.toDouble());
        }
        // Page masquée pendant sa construction : le premier rendu du texte est celui que l'on chronomètre.
        f.page->setVisible(false);
        grab(f);
        core::Page hostile;
        hostile.objects.push_back(hostileText(bluePng));
        f.page->setPage(hostile);
        QVERIFY(QTest::qWaitFor([&f] { return f.page->isReady(); }, 5000));
        QElapsedTimer timer;
        timer.start();
        f.page->setVisible(true);
        const QImage image = f.window.grabWindow();
        const qint64 sceneMs = timer.elapsed();
        const double ratio = f.window.devicePixelRatio();
        const QRect zone(QPoint(100, 100), QPoint(500, 220));
        QCOMPARE(countPixels(image, zone, ratio, isBlue), 0);
        QVERIFY2(sceneMs < 1000, qPrintable(QString::number(sceneMs)));
        const int red = redPixels(image, QRect(QPoint(100, 100), QPoint(400, 160)), ratio);
        QVERIFY2(red > 200, qPrintable(QStringLiteral("%1 rouges, %2 x %3 à %4")
                                           .arg(red)
                                           .arg(image.width())
                                           .arg(image.height())
                                           .arg(ratio)));
        // Chemin RHI : même peintre, rastérisé à l'échelle de l'écran.
        timer.restart();
        QRectF rect;
        const QImage raster = render::rasterizeLocal(hostileText(bluePng), kNoImages, 2.0, &rect);
        const qint64 rasterMs = timer.elapsed();
        QCOMPARE(countPixels(raster, raster.rect(), 1, isBlue), 0);
        QVERIFY2(rasterMs < 1000, qPrintable(QString::number(rasterMs)));
        QVERIFY(redPixels(raster, raster.rect()) > 200);
    }
};

QTEST_MAIN(TstRenderPainted)
#include "tst_render_painted.moc"
