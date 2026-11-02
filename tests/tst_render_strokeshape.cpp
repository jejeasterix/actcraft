// Forme du trait (D2) et marqueur d'un seul tenant (D3), L3-T11. Chaque cas est rendu par les chemins qui le
// portent : scène hors écran (moteur logiciel de Qt), scène dont les objets peints sont rastérisés en
// textures (setForceRasterized, chemin RHI du moteur peint), peintre partagé (renderPage), géométrie de
// triangles (chemin RHI du stylo, rastérisée ici par QPainter : hors écran, Qt ne prend que le moteur
// logiciel), encre vive.
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/render/paintedrenderer.h>
#include <actcraft/render/strokegeometry.h>

#include <QImage>
#include <QPainter>
#include <QPolygonF>
#include <QQuickWindow>
#include <QSGGeometryNode>
#include <QTest>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>

using namespace actcraft;

namespace {

enum class Engine
{
    Scene,
    SceneRasterized,
    Painter,
};

const char* nameOf(Engine engine)
{
    switch (engine) {
    case Engine::Scene:
        return "scène";
    case Engine::SceneRasterized:
        return "scène rastérisée";
    case Engine::Painter:
        return "peintre";
    }
    return "?";
}

const render::ImageLookup kNoImages = [](const std::string&) { return QImage(); };

bool isDark(QColor c)
{
    return c.red() < 100 && c.green() < 100 && c.blue() < 100;
}

bool isLight(QColor c)
{
    return c.red() > 200 && c.green() > 200 && c.blue() > 200;
}

int channelGap(QColor a, QColor b)
{
    return std::max(
        {std::abs(a.red() - b.red()), std::abs(a.green() - b.green()), std::abs(a.blue() - b.blue())});
}

QString describe(const char* what, Engine engine, QColor c)
{
    return QStringLiteral("%1 (%2) : %3 %4 %5")
        .arg(QString::fromUtf8(what), QString::fromUtf8(nameOf(engine)))
        .arg(c.red())
        .arg(c.green())
        .arg(c.blue());
}

QString describePair(QColor live, QColor folded)
{
    return QStringLiteral("encre vive %1 %2 %3 / versé %4 %5 %6")
        .arg(live.red())
        .arg(live.green())
        .arg(live.blue())
        .arg(folded.red())
        .arg(folded.green())
        .arg(folded.blue());
}

core::Page pageWith(std::vector<core::PageObject> objects)
{
    core::Page page;
    page.objects = std::move(objects);
    return page;
}

// Page entière (1024 × 768, une unité de page par pixel) sur fond blanc.
QImage drawPage(const core::Page& page, Engine engine)
{
    if (engine == Engine::Painter) {
        return render::renderPage(page, kNoImages, 1.0).convertToFormat(QImage::Format_RGB32);
    }
    QQuickWindow window;
    window.setColor(Qt::white);
    window.resize(1024, 768);
    auto* item = new render::FixedPageItem;
    item->setSize(QSizeF(1024, 768));
    item->setParentItem(window.contentItem());
    item->setForceRasterized(engine == Engine::SceneRasterized);
    window.show();
    if (!QTest::qWaitForWindowExposed(&window)) {
        return {};
    }
    item->setPage(page);
    if (!QTest::qWaitFor([item] { return item->isReady(); }, 5000)) {
        return {};
    }
    return window.grabWindow().convertToFormat(QImage::Format_RGB32);
}

// Triangles opaques noirs sur blanc, sans lissage : ce que la géométrie couvre.
QImage drawTriangles(const std::vector<render::Vertex2D>& vertices)
{
    QImage image(1024, 768, QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    for (std::size_t i = 0; i + 2 < vertices.size(); i += 3) {
        const QPolygonF triangle{QPointF(vertices[i].x, vertices[i].y),
                                 QPointF(vertices[i + 1].x, vertices[i + 1].y),
                                 QPointF(vertices[i + 2].x, vertices[i + 2].y)};
        painter.drawPolygon(triangle);
    }
    painter.end();
    return image;
}

// Encre vive : le trait en cours, sans versement, sur fond blanc.
QImage drawInk(const std::vector<core::StrokePoint>& points, core::Argb color, double width)
{
    QQuickWindow window;
    window.setColor(Qt::white);
    window.resize(1024, 768);
    auto* ink = new render::InkItem;
    ink->setSize(QSizeF(1024, 768));
    ink->setParentItem(window.contentItem());
    window.show();
    if (!QTest::qWaitForWindowExposed(&window)) {
        return {};
    }
    ink->beginStroke(color, width);
    std::uint64_t seq = 0;
    for (const core::StrokePoint& point : points) {
        ink->addPoint(point, ++seq);
    }
    return window.grabWindow().convertToFormat(QImage::Format_RGB32);
}

// Encre vive, chemin RHI (hors écran, Qt n'y passe pas), rendu comme le ferait la carte : triangles des blocs
// (syncRhiBlocks) dans la couleur que l'élément leur donne ; si la couche de l'élément est active, composés
// d'un seul tenant sous l'opacité de l'élément, sinon chaque triangle fondu à part sous cette opacité.
QImage drawInkRhi(const std::vector<core::StrokePoint>& points, core::Argb color, double width)
{
    render::InkItem ink;
    ink.beginStroke(color, width);
    render::InkBuffer buffer;
    buffer.begin(ink.drawnColor(), width);
    for (const core::StrokePoint& point : points) {
        buffer.add(point);
    }
    QSGNode root;
    render::InkItem::syncRhiBlocks(&root, buffer);
    const QObject* layer = ink.property("layer").value<QObject*>();
    const bool layered = layer && layer->property("enabled").toBool();
    QImage page(1024, 768, QImage::Format_ARGB32_Premultiplied);
    page.fill(Qt::white);
    QImage offscreen(page.size(), QImage::Format_ARGB32_Premultiplied);
    offscreen.fill(Qt::transparent);
    {
        QPainter painter(layered ? &offscreen : &page);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor::fromRgba(ink.drawnColor()));
        painter.setOpacity(layered ? 1.0 : ink.opacity());
        for (QSGNode* child = root.firstChild(); child; child = child->nextSibling()) {
            const QSGGeometry* geometry = static_cast<QSGGeometryNode*>(child)->geometry();
            const auto* v = static_cast<const render::Vertex2D*>(geometry->vertexData());
            for (int i = 0; i + 2 < geometry->vertexCount(); i += 3) {
                const QPolygonF triangle{QPointF(v[i].x, v[i].y), QPointF(v[i + 1].x, v[i + 1].y),
                                         QPointF(v[i + 2].x, v[i + 2].y)};
                painter.drawPolygon(triangle);
            }
        }
    }
    if (layered) {
        QPainter painter(&page);
        painter.setOpacity(ink.opacity());
        painter.drawImage(0, 0, offscreen);
    }
    return page.convertToFormat(QImage::Format_RGB32);
}

// Boucle qui se recoupe : trois branches droites à points serrés (le lissage garde les branches droites et
// n'arrondit que les deux coins). Croisement en (150, 175) ; (100, 137) sur la première branche seule.
std::vector<core::StrokePoint> selfCrossingLoop()
{
    std::vector<core::StrokePoint> out;
    const auto run = [&out](core::PointF from, core::PointF to, int steps) {
        for (int i = out.empty() ? 0 : 1; i <= steps; ++i) {
            const double t = static_cast<double>(i) / steps;
            out.push_back({from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t, 0.5f});
        }
    };
    run({50, 100}, {250, 250}, 20);
    run({250, 250}, {250, 100}, 15);
    run({250, 100}, {50, 250}, 20);
    return out;
}

constexpr QPoint kCrossing{150, 175};
constexpr QPoint kBranch{100, 137};

// Marqueur jaune versé comme le fait le stylo : couleur opaque, translucidité 127 (opacité 128 sur 255, D3).
core::PageObject highlighter(std::vector<core::StrokePoint> points)
{
    core::Stroke s;
    s.kind = core::StrokeKind::Highlighter;
    s.cap = core::PenCap::Round;
    s.baseWidth = 20;
    s.color = 0xFFFFFF00u;
    s.points = std::move(points);
    core::PageObject object(s);
    object.common.translucency = 127;
    return object;
}

} // namespace

class TstRenderStrokeshape : public QObject
{
    Q_OBJECT

private slots:
    // Trait horizontal d'épaisseur 20 de (100, 150) à (200, 150). Bout rond : à 8 px au-delà, sur l'axe,
    // encré ; à 8 px au-delà et 9 px de l'axe (12,7 px du bout), blanc. Bout carré du prototype : les deux
    // encrés.
    void roundCapsCoverTheDisk()
    {
        core::Stroke s;
        s.baseWidth = 20;
        s.color = 0xFF000000u;
        s.points = {{100, 150, 0.5f}, {200, 150, 0.5f}};
        const QPoint onAxis(208, 150);
        const QPoint offAxis(208, 159);
        for (const core::PenCap cap : {core::PenCap::Round, core::PenCap::Square}) {
            s.cap = cap;
            const bool rounded = cap == core::PenCap::Round;
            for (const Engine engine : {Engine::Scene, Engine::Painter}) {
                const QImage image = drawPage(pageWith({core::PageObject(s)}), engine);
                QVERIFY2(!image.isNull(), nameOf(engine));
                QVERIFY2(isDark(image.pixelColor(onAxis)),
                         qPrintable(describe("sur l'axe", engine, image.pixelColor(onAxis))));
                const QColor off = image.pixelColor(offAxis);
                QVERIFY2(rounded ? isLight(off) : isDark(off), qPrintable(describe("coin", engine, off)));
            }
            std::vector<render::Vertex2D> vertices;
            if (rounded) {
                render::appendRoundStrokeTriangles(s.points, s.baseWidth, vertices);
            } else {
                render::appendStrokeTriangles(s.points, s.baseWidth, vertices);
            }
            const QImage geometry = drawTriangles(vertices);
            QVERIFY(isDark(geometry.pixelColor(onAxis)));
            QVERIFY(rounded ? isLight(geometry.pixelColor(offAxis)) : isDark(geometry.pixelColor(offAxis)));
        }
    }

    // Trait en V (100, 300) → (200, 100) → (300, 300), épaisseur 20 : la courbe passe par le milieu (250,
    // 200) du second segment et laisse le sommet (200, 100) loin d'elle (à 70 px environ). Le sommet et le
    // pixel à 8 px au-dessus, sur la bissectrice, sont blancs ; une ligne brisée les encrerait, même à
    // jonction ronde. Les bouts restent aux points d'origine.
    void midpointSmoothing()
    {
        core::Stroke s;
        s.baseWidth = 20;
        s.color = 0xFF000000u;
        s.cap = core::PenCap::Round;
        s.points = {{100, 300, 0.5f}, {200, 100, 0.5f}, {300, 300, 0.5f}};
        const std::vector<QPoint> blank{{200, 100}, {200, 92}};
        // Bouts, bout rond derrière le premier point, et la courbe à t = 0,5 du premier arc : (187,5 ; 175).
        const std::vector<QPoint> inked{{100, 300}, {300, 300}, {96, 307}, {187, 175}};
        std::vector<render::Vertex2D> vertices;
        render::appendRoundStrokeTriangles(s.points, s.baseWidth, vertices);
        const QImage geometry = drawTriangles(vertices);
        for (const Engine engine : {Engine::Scene, Engine::Painter}) {
            const QImage image = drawPage(pageWith({core::PageObject(s)}), engine);
            QVERIFY2(!image.isNull(), nameOf(engine));
            for (const QPoint& p : blank) {
                QVERIFY2(isLight(image.pixelColor(p)),
                         qPrintable(describe("sommet", engine, image.pixelColor(p))));
                QVERIFY(isLight(geometry.pixelColor(p)));
            }
            for (const QPoint& p : inked) {
                QVERIFY2(isDark(image.pixelColor(p)),
                         qPrintable(describe("trait", engine, image.pixelColor(p))));
                QVERIFY(isDark(geometry.pixelColor(p)));
            }
        }
        const std::vector<core::PointF> line = render::smoothedStrokePoints(s.points);
        QCOMPARE(line.front().x, 100.0);
        QCOMPARE(line.front().y, 300.0);
        QCOMPARE(line.back().x, 300.0);
        QCOMPARE(line.back().y, 300.0);
    }

    // Amendement 2 de l'orchestrateur : un stylo opaque à bouts ronds n'est plus envoyé au moteur peint ; un
    // trait translucide, si (texture d'un seul drawPath). D2 : le bout d'un fichier est ignoré (plat → forme
    // ronde).
    void roundPenStrokeStaysOnTheGeometryPath()
    {
        core::Stroke pen;
        pen.cap = core::PenCap::Round;
        pen.points = {{0, 0, 0.5f}, {10, 10, 0.5f}};
        QVERIFY(render::hasRoundShape(pen));
        QVERIFY(!render::usesPaintedRenderer(core::PageObject(pen)));
        core::Stroke flat = pen;
        flat.cap = core::PenCap::Flat;
        QVERIFY(render::hasRoundShape(flat));
        QVERIFY(!render::usesPaintedRenderer(core::PageObject(flat)));
        QVERIFY(!render::hasRoundShape(core::Stroke{}));
        QVERIFY(!render::usesPaintedRenderer(core::PageObject(core::Stroke{})));
        core::PageObject faded(pen);
        faded.common.translucency = 127;
        QVERIFY(render::usesPaintedRenderer(faded));
        core::Stroke tinted = pen;
        tinted.color = 0x80000000u;
        QVERIFY(render::usesPaintedRenderer(core::PageObject(tinted)));
        core::Stroke marker = pen;
        marker.kind = core::StrokeKind::Highlighter;
        QVERIFY(render::usesPaintedRenderer(core::PageObject(marker)));
        std::vector<render::Vertex2D> vertices;
        QVERIFY(render::appendRoundStrokeTriangles(pen.points, 8, vertices) > 0);
        QCOMPARE(vertices.size() % 3, std::size_t{0});
    }

    // Marqueur translucide 128 en boucle qui se recoupe : au croisement, la même couleur qu'au milieu d'une
    // branche (écart ≤ 2 par canal), sur la page fixe (les deux moteurs), au peintre et dans l'encre vive.
    void selfCrossingHighlighterDoesNotDarken()
    {
        const std::vector<core::StrokePoint> loop = selfCrossingLoop();
        for (const Engine engine : {Engine::Scene, Engine::SceneRasterized, Engine::Painter}) {
            const QImage image = drawPage(pageWith({highlighter(loop)}), engine);
            QVERIFY2(!image.isNull(), nameOf(engine));
            const QColor branch = image.pixelColor(kBranch);
            const QColor crossing = image.pixelColor(kCrossing);
            QVERIFY2(branch.blue() < 200 && branch.red() > 200,
                     qPrintable(describe("branche", engine, branch)));
            QVERIFY2(channelGap(branch, crossing) <= 2, qPrintable(describe("croisement", engine, crossing)));
        }
        const QImage ink = drawInk(loop, 0x80FFFF00u, 20);
        QVERIFY(!ink.isNull());
        const QColor branch = ink.pixelColor(kBranch);
        const QColor crossing = ink.pixelColor(kCrossing);
        QVERIFY2(branch.blue() < 200 && branch.red() > 200,
                 qPrintable(describe("branche", Engine::Scene, branch)));
        QVERIFY2(channelGap(branch, crossing) <= 2,
                 qPrintable(describe("croisement de l'encre vive", Engine::Scene, crossing)));
        // Encre vive, chemin RHI : le moteur logiciel peint chaque suite de segments d'un seul chemin et ne
        // fonce donc pas, même sous une couleur à alpha ; ce sont les triangles des blocs qui foncent.
        const QImage rhi = drawInkRhi(loop, 0x80FFFF00u, 20);
        const QColor rhiBranch = rhi.pixelColor(kBranch);
        const QColor rhiCrossing = rhi.pixelColor(kCrossing);
        QVERIFY2(rhiBranch.blue() < 200 && rhiBranch.red() > 200,
                 qPrintable(describe("branche de l'encre vive RHI", Engine::Scene, rhiBranch)));
        QVERIFY2(channelGap(rhiBranch, rhiCrossing) <= 2,
                 qPrintable(describe("croisement de l'encre vive RHI", Engine::Scene, rhiCrossing)));
    }

    // Deux traits de marqueur superposés foncent : pixel commun plus sombre qu'un pixel d'un seul trait
    // (écart ≥ 20 sur le bleu, fond blanc). Garde contre une couche d'un seul tenant posée sur toute la page.
    void twoHighlighterPassesDarken()
    {
        const core::PageObject across = highlighter({{50, 200, 0.5f}, {350, 200, 0.5f}});
        const core::PageObject down = highlighter({{200, 100, 0.5f}, {200, 300, 0.5f}});
        for (const Engine engine : {Engine::Scene, Engine::SceneRasterized, Engine::Painter}) {
            const QImage image = drawPage(pageWith({across, down}), engine);
            QVERIFY2(!image.isNull(), nameOf(engine));
            const QColor single = image.pixelColor(100, 200);
            const QColor both = image.pixelColor(200, 200);
            QVERIFY2(single.blue() < 200, qPrintable(describe("un trait", engine, single)));
            QVERIFY2(both.blue() <= single.blue() - 20, qPrintable(describe("deux traits", engine, both)));
        }
    }

    // Le même marqueur dessiné par l'encre vive puis versé dans la page fixe : mêmes pixels au croisement et
    // sur une branche (écart ≤ 4).
    void liveInkMatchesTheFoldedStroke()
    {
        const std::vector<core::StrokePoint> loop = selfCrossingLoop();
        const QImage ink = drawInk(loop, 0x80FFFF00u, 20);
        const QImage folded = drawPage(pageWith({highlighter(loop)}), Engine::Scene);
        QVERIFY(!ink.isNull() && !folded.isNull());
        for (const QPoint& p : {kCrossing, kBranch}) {
            QVERIFY2(channelGap(ink.pixelColor(p), folded.pixelColor(p)) <= 4,
                     qPrintable(describePair(ink.pixelColor(p), folded.pixelColor(p))));
        }
    }

    // Encre vive à forme ronde (D2), avant tout versement : bouts ronds (comme roundCapsCoverTheDisk) et
    // courbe par les milieux (comme midpointSmoothing ; le dernier segment, provisoire, est droit comme celui
    // du trait versé).
    void liveInkHasRoundCapsAndSmoothing()
    {
        const QImage line = drawInk({{100, 150, 0.5f}, {200, 150, 0.5f}}, 0xFF000000u, 20);
        QVERIFY(!line.isNull());
        QVERIFY2(isDark(line.pixelColor(208, 150)),
                 qPrintable(describe("sur l'axe", Engine::Scene, line.pixelColor(208, 150))));
        QVERIFY2(isLight(line.pixelColor(208, 159)),
                 qPrintable(describe("coin", Engine::Scene, line.pixelColor(208, 159))));
        const QImage vee = drawInk({{100, 300, 0.5f}, {200, 100, 0.5f}, {300, 300, 0.5f}}, 0xFF000000u, 20);
        QVERIFY(!vee.isNull());
        for (const QPoint& p : {QPoint(200, 100), QPoint(200, 92)}) {
            QVERIFY2(isLight(vee.pixelColor(p)),
                     qPrintable(describe("sommet", Engine::Scene, vee.pixelColor(p))));
        }
        for (const QPoint& p : {QPoint(100, 300), QPoint(96, 307), QPoint(187, 175), QPoint(300, 300)}) {
            QVERIFY2(isDark(vee.pixelColor(p)),
                     qPrintable(describe("trait", Engine::Scene, vee.pixelColor(p))));
        }
    }

    // Chemin RHI de l'encre vive (hors écran, Qt n'y passe pas) : un trait translucide est dessiné opaque,
    // dans la couche de l'élément, sous l'opacité de la couleur ; clear() rend l'élément opaque, sans couche.
    void liveTranslucentInkDrawsOpaqueInALayer()
    {
        render::InkItem ink;
        QObject* layer = ink.property("layer").value<QObject*>();
        QVERIFY(layer);
        ink.beginStroke(0x80FFFF00u, 20);
        QVERIFY(ink.isTranslucent());
        QCOMPARE(ink.drawnColor(), core::Argb{0xFFFFFF00u});
        QVERIFY(std::abs(ink.opacity() - 128.0 / 255.0) < 1e-6);
        QVERIFY(layer->property("enabled").toBool());
        ink.clear();
        QVERIFY(!ink.isTranslucent());
        QCOMPARE(ink.opacity(), 1.0);
        QVERIFY(!layer->property("enabled").toBool());
        ink.beginStroke(0xFF000000u, 3);
        QVERIFY(!ink.isTranslucent());
        QCOMPARE(ink.drawnColor(), core::Argb{0xFF000000u});
        QVERIFY(!layer->property("enabled").toBool());
    }
};

QTEST_MAIN(TstRenderStrokeshape)
#include "tst_render_strokeshape.moc"
