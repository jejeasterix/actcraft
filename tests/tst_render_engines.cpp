#include "support/renderhost.h"

#include <actcraft/render/objectrenderer.h>
#include <actcraft/render/strokegeometry.h>

#include <QSignalSpy>
#include <QTest>

#include <algorithm>

using namespace actcraft;

namespace {

QImage renderObjects(std::vector<core::PageObject> objects)
{
    QQuickWindow window;
    window.setColor(Qt::white);
    window.resize(300, 300);
    auto* host = new RenderHostItem;
    host->setSize(QSizeF(300, 300));
    host->objects = std::move(objects);
    host->setParentItem(window.contentItem());
    window.show();
    if (!QTest::qWaitForWindowExposed(&window)) {
        return {};
    }
    return window.grabWindow();
}

core::Shape shape(core::ShapeKind kind, core::RectF bounds, core::Argb fill, double opacity = 1.0)
{
    core::Shape s;
    s.kind = kind;
    s.bounds = bounds;
    s.fill = fill;
    s.outline = fill;
    s.outlineWidth = 0;
    s.opacity = opacity;
    return s;
}

} // namespace

class TstRenderEngines : public QObject
{
    Q_OBJECT

private slots:
    // Rendu logiciel : les nœuds recréés à la mise à jour restent sous la transformation de l'élément.
    void updatedHostKeepsTransform()
    {
        QQuickWindow window;
        window.setColor(Qt::white);
        window.resize(800, 600);
        auto* host = new RenderHostItem;
        host->setSize(QSizeF(300, 200));
        host->setPosition(QPointF(100, 50));
        host->setTransformOrigin(QQuickItem::TopLeft);
        host->setScale(2.0);
        const auto line = [](double y) {
            core::Stroke stroke;
            stroke.baseWidth = 6;
            stroke.color = 0xFF000000u;
            stroke.points = {{10, y, 0.5f}, {60, y, 0.5f}, {110, y, 0.5f}};
            return stroke;
        };
        host->objects.emplace_back(line(10));
        host->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QSignalSpy swapped(&window, &QQuickWindow::frameSwapped);
        host->update();
        QVERIFY(swapped.wait(2000));
        host->objects.emplace_back(line(40));
        host->update();
        const QImage image = window.grabWindow();
        QVERIFY2(colorNear(image.pixelColor(220, 70), QColor(Qt::black), 40), "trait 1");
        QVERIFY2(colorNear(image.pixelColor(220, 130), QColor(Qt::black), 40), "trait 2");
        QVERIFY2(colorNear(image.pixelColor(60, 10), QColor(Qt::white), 1), "sans transformation");
    }
    void registryHasShapeAndStroke()
    {
        const auto registry = render::RendererRegistry::withDefaults();
        QVERIFY(registry.find(core::ObjectKind::Shape));
        QVERIFY(registry.find(core::ObjectKind::Stroke));
        QCOMPARE(registry.find(core::ObjectKind::Shape)->kind(), core::ObjectKind::Shape);
        QCOMPARE(registry.find(core::ObjectKind::Stroke)->kind(), core::ObjectKind::Stroke);
    }

    void horizontalSegmentIsAQuad()
    {
        const std::vector<core::StrokePoint> points{{0, 0, 0.5f}, {10, 0, 0.5f}};
        std::vector<render::Vertex2D> out;
        QCOMPARE(render::appendStrokeTriangles(points, 4.0, out), render::kVerticesPerSegment);
        QCOMPARE(out.size(), render::kVerticesPerSegment);
        float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
        for (const auto& v : out) {
            minX = std::min(minX, v.x);
            maxX = std::max(maxX, v.x);
            minY = std::min(minY, v.y);
            maxY = std::max(maxY, v.y);
        }
        QCOMPARE(minX, -2.0f); // bout carré : prolongé d'une demi-largeur
        QCOMPARE(maxX, 12.0f);
        QCOMPARE(minY, -2.0f); // largeur 4 × (0,5 + 0,5)
        QCOMPARE(maxY, 2.0f);
    }

    void degenerateSegmentKeepsVertexCount()
    {
        const std::vector<core::StrokePoint> points{{5, 5, 0.5f}, {5, 5, 0.5f}, {6, 5, 0.5f}};
        std::vector<render::Vertex2D> out;
        QCOMPARE(render::appendStrokeTriangles(points, 3.0, out), 2 * render::kVerticesPerSegment);
    }

    void fillAndOutlineCounts()
    {
        const auto square = render::rectanglePolygon({0, 0, 10, 10});
        QCOMPARE(square.size(), std::size_t{4});
        std::vector<render::Vertex2D> fill;
        QCOMPARE(render::appendConvexFill(square, fill), std::size_t{6});
        std::vector<render::Vertex2D> outline;
        QCOMPARE(render::appendOutlineTriangles(square, 2.0, outline), 4 * render::kVerticesPerSegment);
    }

    void ellipseInsideBounds()
    {
        const auto polygon = render::ellipsePolygon({10, 20, 100, 50}, 48);
        QCOMPARE(polygon.size(), std::size_t{48});
        for (const auto& p : polygon) {
            QVERIFY(p.x >= 10 - 1e-9 && p.x <= 110 + 1e-9);
            QVERIFY(p.y >= 20 - 1e-9 && p.y <= 70 + 1e-9);
        }
    }

    void strokeBoundsInflated()
    {
        const std::vector<core::StrokePoint> points{{10, 10, 1.0f}, {20, 30, 0.0f}};
        const core::RectF r = render::strokeBounds(points, 4.0); // demi-largeur max 3
        QCOMPARE(r.x, 7.0);
        QCOMPARE(r.y, 7.0);
        QCOMPARE(r.width, 16.0);
        QCOMPARE(r.height, 26.0);
    }

    void shapesRenderInSoftware()
    {
        core::Shape triangle = shape(core::ShapeKind::Polygon, {20, 120, 80, 80}, 0xFFE53935u);
        triangle.polygon = {{20, 200}, {100, 200}, {60, 120}};
        const QImage image = renderObjects({
            shape(core::ShapeKind::Rectangle, {20, 20, 80, 60}, 0xFF1E88E5u),
            shape(core::ShapeKind::Ellipse, {120, 20, 80, 60}, 0xFF43A047u),
            triangle,
            shape(core::ShapeKind::Rectangle, {120, 120, 80, 80}, 0xFF000000u, 0.5),
        });
        QVERIFY(!image.isNull());
        QVERIFY(colorNear(image.pixelColor(60, 50), QColor(0x1E, 0x88, 0xE5), 3));
        QVERIFY(colorNear(image.pixelColor(160, 50), QColor(0x43, 0xA0, 0x47), 3));
        QVERIFY(colorNear(image.pixelColor(60, 180), QColor(0xE5, 0x39, 0x35), 3));
        QVERIFY(colorNear(image.pixelColor(160, 160), QColor(127, 127, 127), 6)); // 50 % de noir sur blanc
        QVERIFY(colorNear(image.pixelColor(250, 250), QColor(Qt::white), 1));
    }

    void strokeRendersInSoftware()
    {
        core::Stroke stroke;
        stroke.baseWidth = 8;
        stroke.color = 0xFF000000u;
        stroke.points = {{20, 250, 0.5f}, {150, 250, 0.5f}, {280, 250, 0.5f}};
        const QImage image = renderObjects({stroke});
        QVERIFY(colorNear(image.pixelColor(100, 250), QColor(Qt::black), 10));
        QVERIFY(colorNear(image.pixelColor(200, 250), QColor(Qt::black), 10));
        QVERIFY(colorNear(image.pixelColor(100, 270), QColor(Qt::white), 1));
    }

    void diagonalStrokeKeepsSquareCapCorners()
    {
        // Largeur 40 (demi-largeur 20), diagonale à 45° : les coins du bout carré dépassent de strokeBounds
        // (de (√2 − 1) × demi-largeur, soit ~8 px) ; le moteur logiciel découpe au rect() du nœud.
        core::Stroke stroke;
        stroke.baseWidth = 40;
        stroke.color = 0xFF000000u;
        stroke.points = {{100, 100, 0.5f}, {150, 150, 0.5f}};
        const core::RectF bounds = render::strokeBounds(stroke.points, stroke.baseWidth);
        QVERIFY(176 > bounds.y + bounds.height); // le coin (150 ; 178,3) est hors des bornes
        const QImage image = renderObjects({stroke});
        QVERIFY(!image.isNull());
        QVERIFY(colorNear(image.pixelColor(150, 175), QColor(Qt::black), 10));
        QVERIFY(colorNear(image.pixelColor(175, 150), QColor(Qt::black), 10));
        QVERIFY(colorNear(image.pixelColor(125, 125), QColor(Qt::black), 10));
    }
};

QTEST_MAIN(TstRenderEngines)
#include "tst_render_engines.moc"
