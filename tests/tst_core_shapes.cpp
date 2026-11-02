#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/shapeedits.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/core/undo.h>

#include <QtTest>

#include <cmath>

using namespace actcraft::core;

namespace {

bool samePoints(const std::vector<PointF>& a, const std::vector<PointF>& b)
{
    if (a.size() != b.size())
        return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].x != b[i].x || a[i].y != b[i].y)
            return false;
    }
    return true;
}

} // namespace

class TstCoreShapes : public QObject
{
    Q_OBJECT

private slots:
    void catalogOrderAndDefault()
    {
        const auto& catalog = shapeCatalog();
        QCOMPARE(catalog.size(), std::size_t{22});
        QCOMPARE(QString::fromStdString(catalog.front().id), QStringLiteral("line"));
        QCOMPARE(QString::fromStdString(catalog[3].id), QStringLiteral("freeform")); // D1, amendement 4
        QCOMPARE(QString::fromStdString(catalog[4].id), QStringLiteral("polyline"));
        QCOMPARE(QString::fromStdString(catalog[5].id), QStringLiteral("curve-chain"));
        QCOMPARE(QString::fromStdString(catalog[6].id), QStringLiteral("rectangle"));
        QVERIFY(findShapePreset("zigzag") == nullptr);
        QVERIFY(findShapePreset("chevron") == nullptr);
        QVERIFY(findShapePreset("circle")->square);
        QVERIFY(findShapePreset("nothing") == nullptr);
        for (const auto& preset : catalog) {
            const PageObject o = preset.family == ShapeFamily::Line
                                     ? makeLineObject(preset.id, {0, 0}, {100, 60}, {})
                                     : makeShapeObject(preset.id, {0, 0, 100, 60}, {});
            QCOMPARE(o.common.layer, Layer::Middle);
            if (preset.family == ShapeFamily::Line)
                QVERIFY2(std::get<Line>(body(o)).points.size() >= 2, preset.id.c_str());
            else
                QVERIFY2(!std::get<Shape>(body(o)).polygon.empty() || !std::get<Shape>(body(o)).paths.empty(),
                         preset.id.c_str());
        }
    }
    void dragFromCornerAndCenter()
    {
        const RectF corner = dragBox({10, 20}, {110, 80}, false, false);
        QCOMPARE(corner.x, 10.0);
        QCOMPARE(corner.width, 100.0);
        const RectF backwards = dragBox({110, 80}, {10, 20}, false, false);
        QCOMPARE(backwards.x, 10.0);
        QCOMPARE(backwards.height, 60.0);
        const RectF center = dragBox({100, 100}, {150, 130}, true, false);
        QCOMPARE(center.x, 50.0);
        QCOMPARE(center.y, 70.0);
        QCOMPARE(center.width, 100.0);
        QCOMPARE(center.height, 60.0);
    }
    void squarePresetsStaySquare()
    {
        const RectF box = dragBox({0, 0}, {100, -30}, false, true);
        QCOMPARE(box.width, 100.0);
        QCOMPARE(box.height, 100.0);
        QCOMPARE(box.y, -100.0);
    }
    void linePresetsConstrained()
    {
        const auto h = dragLine({0, 0}, {100, 40}, *findShapePreset("line-horizontal"), false, false);
        QCOMPARE(h.second.y, 0.0);
        const auto v = dragLine({0, 0}, {100, 40}, *findShapePreset("line-vertical"), false, false);
        QCOMPARE(v.second.x, 0.0);
        const auto d = dragLine({0, 0}, {100, 90}, *findShapePreset("line"), false, true);
        QVERIFY(std::abs(d.second.x - d.second.y) < 1e-9); // 45°
    }
    void arrowHeadSmartPointBounded()
    {
        PageObject o = makeShapeObject("arrow", {0, 0, 200, 100}, {});
        auto& shape = std::get<Shape>(body(o));
        const auto points = smartPoints(shape);
        QVERIFY(!points.empty());
        const auto* preset = findShapePreset("arrow");
        const auto far = dragSmartPoint(shape, 0, {-1000, -1000});
        QCOMPARE(far.size(), preset->minimum.size());
        for (std::size_t i = 0; i < far.size(); ++i) {
            QVERIFY(far[i] >= preset->minimum[i]);
            QVERIFY(far[i] <= preset->maximum[i]);
        }
        PageObject read{Shape{}};
        QVERIFY(smartPoints(std::get<Shape>(body(read))).empty()); // forme lue : aucun
    }
    void arrowHeadStartsAtFourTenths() // D4, amendement 4
    {
        const ShapePreset* arrow = findShapePreset("arrow");
        QVERIFY(arrow != nullptr);
        QCOMPARE(arrow->defaults.size(), std::size_t{2});
        QCOMPARE(arrow->defaults[0], 0.6); // début de la tête : tête de 0,4 de la largeur
        QCOMPARE(arrow->minimum[0], 0.0);
        QCOMPARE(arrow->maximum[0], 1.0);
        QCOMPARE(arrow->minimum[1], 0.0); // épaisseur du corps, fraction de la hauteur
        QCOMPARE(arrow->maximum[1], 0.5);
    }
    void smartPointsOnlyForThreePresets() // D4, amendement 4
    {
        for (const auto& preset : shapeCatalog()) {
            const bool expected =
                preset.id == "rounded-rectangle" || preset.id == "arrow" || preset.id == "callout";
            QVERIFY2(preset.defaults.empty() != expected, preset.id.c_str());
            if (preset.family == ShapeFamily::Closed) {
                PageObject o = makeShapeObject(preset.id, {0, 0, 100, 60}, {});
                QVERIFY2(smartPoints(std::get<Shape>(body(o))).empty() != expected, preset.id.c_str());
            }
        }
        const ShapePreset* rounded = findShapePreset("rounded-rectangle");
        QCOMPARE(rounded->minimum[0], 0.01);
        QCOMPARE(rounded->maximum[0], 0.5);
        const ShapePreset* callout = findShapePreset("callout");
        QVERIFY(callout->defaults == (std::vector<double>{1.0, 0.1}));
        QVERIFY(callout->minimum == (std::vector<double>{0.0, 0.01}));
        QVERIFY(callout->maximum == (std::vector<double>{1.0, 0.3}));
    }
    void gestureOfEachPreset() // D1, amendement 4
    {
        QVERIFY(shapeGesture("freeform") == ShapeGesture::Freehand);
        QVERIFY(shapeGesture("polyline") == ShapeGesture::Clicks);
        QVERIFY(shapeGesture("curve-chain") == ShapeGesture::Clicks);
        QVERIFY(shapeGesture("line") == ShapeGesture::Box);
        QVERIFY(shapeGesture("rectangle") == ShapeGesture::Box);
        QVERIFY(shapeGesture("nothing") == ShapeGesture::Box);
    }
    void setParametersIsOneStepAndRebuilds()
    {
        Document document;
        UndoStack stack;
        stack.push(document,
                   std::make_unique<AddObjectCommand>(0, makeShapeObject("arrow", {0, 0, 200, 100}, {})));
        const ObjectId id = document.page(0).objects.back().common.id;
        const auto before = std::get<Shape>(body(document.page(0).objects.back())).polygon;
        auto params = findShapePreset("arrow")->defaults;
        params[0] = findShapePreset("arrow")->maximum[0];
        const Changes changes =
            stack.push(document, std::make_unique<SetShapeParametersCommand>(0, id, params));
        QCOMPARE(changes.size(), std::size_t{1});
        QCOMPARE(changes[0].kind, Change::Kind::ObjectChanged);
        QVERIFY(!samePoints(std::get<Shape>(body(document.page(0).objects.back())).polygon, before));
        stack.undo(document);
        QVERIFY(samePoints(std::get<Shape>(body(document.page(0).objects.back())).polygon, before));
    }
    void smoothChainPassesThroughVertices() // L4-T31
    {
        const std::vector<PointF> vertices{{0, 0}, {100, 0}, {100, 100}, {200, 100}};
        const std::vector<PointF> curve = smoothChain(vertices, 8);
        QCOMPARE(curve.size(), std::size_t{3 * 8 + 1});
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            QVERIFY(std::abs(curve[i * 8].x - vertices[i].x) < 1e-9);
            QVERIFY(std::abs(curve[i * 8].y - vertices[i].y) < 1e-9);
        }
        bool leavesTheStraightSegment = false; // entre (100, 0) et (100, 100), la courbe n'est pas droite
        for (std::size_t k = 1; k < 8; ++k)
            leavesTheStraightSegment = leavesTheStraightSegment || std::abs(curve[8 + k].x - 100) > 1e-6;
        QVERIFY(leavesTheStraightSegment);
        QCOMPARE(smoothChain({{0, 0}, {10, 0}}, 8).size(), std::size_t{2});
    }
    void chainNeedsTwoDistinctPoints() // L4-T31
    {
        QVERIFY(!makeChainObject("polyline", {{10, 10}}, {}).has_value());
        QVERIFY(!makeChainObject("polyline", {{10, 10}, {10, 10}}, {}).has_value());
        QVERIFY(!makeChainObject("rectangle", {{0, 0}, {50, 50}}, {}).has_value()); // geste Box
        const std::optional<PageObject> o =
            makeChainObject("polyline", {{0, 0}, {0, 0}, {50, 0}, {50, 40}}, {});
        QVERIFY(o.has_value());
        QCOMPARE(o->common.layer, Layer::Middle);
        QCOMPARE(std::get<Line>(body(*o)).points.size(), std::size_t{3}); // doublon retiré
        QVERIFY(makeChainObject("freeform", {{0, 0}, {3, 1}, {6, 4}}, {}).has_value());
        QVERIFY(std::get<Line>(body(*makeChainObject("curve-chain", {{0, 0}, {50, 0}, {50, 50}}, {})))
                    .points.size() == std::size_t{2 * kCurveChainSamples + 1});
    }
};

QTEST_GUILESS_MAIN(TstCoreShapes)
#include "tst_core_shapes.moc"
