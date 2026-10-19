#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>

#include <QTest>

using namespace actcraft::core;

namespace {
PageObject strokeObject(ObjectId id, std::vector<StrokePoint> points, double width)
{
    Stroke s;
    s.points = std::move(points);
    s.baseWidth = width;
    PageObject o{s};
    o.common.id = id;
    o.common.layer = Layer::Top;
    return o;
}
PageObject rectObject(ObjectId id, RectF bounds, Layer layer = Layer::Middle)
{
    Shape shape;
    shape.bounds = bounds;
    PageObject o{shape};
    o.common.id = id;
    o.common.layer = layer;
    return o;
}
} // namespace

class TstCoreHittest : public QObject
{
    Q_OBJECT

private slots:
    void topmostObjectWins()
    {
        Page page;
        insertAtTopOfLayer(page, rectObject(1, {100, 100, 200, 200}));
        insertAtTopOfLayer(page, rectObject(2, {150, 150, 200, 200}));
        QCOMPARE(objectAt(page, {180, 180}, 0).value_or(0), ObjectId{2});
        QCOMPARE(objectAt(page, {120, 120}, 0).value_or(0), ObjectId{1});
        QVERIFY(!objectAt(page, {10, 10}, 0).has_value());
    }
    void strokeHitUsesWidthAndTolerance()
    {
        Page page;
        insertAtTopOfLayer(page, strokeObject(7, {{0, 100, 0.5f}, {400, 100, 0.5f}}, 10));
        QCOMPARE(objectAt(page, {200, 104}, 0).value_or(0), ObjectId{7}); // dans la demi-épaisseur (5)
        QVERIFY(!objectAt(page, {200, 112}, 0).has_value());
        QCOMPARE(objectAt(page, {200, 112}, 8).value_or(0), ObjectId{7}); // grâce à la tolérance
        QVERIFY(!objectAt(page, {200, 150}, 8).has_value());              // dans l'emprise, loin de l'encre
    }
    void rectangleTouchingSelects()
    {
        Page page;
        insertAtTopOfLayer(page, rectObject(1, {100, 100, 50, 50}));
        insertAtTopOfLayer(page, rectObject(2, {400, 400, 50, 50}));
        insertAtTopOfLayer(page, strokeObject(3, {{120, 300, 0.5f}, {600, 300, 0.5f}}, 4));
        const auto ids = objectsTouching(page, normalizedRect({140, 140}, {130, 310}));
        QCOMPARE(ids, (std::vector<ObjectId>{1, 3})); // touche le coin du 1 et le trait, pas le 2
    }
    void equationHitsLikeText()
    {
        // C4062 : l'équation (L0-S3) se désigne par son emprise, comme un texte.
        Page page;
        Equation equation;
        equation.bounds = {100, 100, 80, 40};
        PageObject o{equation};
        o.common.id = 9;
        insertAtTopOfLayer(page, o);
        QCOMPARE(objectAt(page, {120, 120}, 0).value_or(0), ObjectId{9});
        QCOMPARE(objectsTouching(page, normalizedRect({170, 130}, {200, 160})), (std::vector<ObjectId>{9}));
    }
    void annotationsAreTopLayerStrokes()
    {
        QVERIFY(isAnnotation(strokeObject(1, {{0, 0, 0.5f}, {1, 1, 0.5f}}, 2)));
        PageObject moved = strokeObject(2, {{0, 0, 0.5f}, {1, 1, 0.5f}}, 2);
        moved.common.layer = Layer::Middle;
        QVERIFY(!isAnnotation(moved));
        QVERIFY(!isAnnotation(rectObject(3, {0, 0, 1, 1}, Layer::Top)));
    }
};

QTEST_GUILESS_MAIN(TstCoreHittest)
#include "tst_core_hittest.moc"
