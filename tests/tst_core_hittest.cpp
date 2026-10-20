#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/objectedits.h>

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
        QVERIFY(!objectAt(page, {200, 150}, 8)
                     .has_value()); // loin de l'encre, hors de l'emprise aussi (87 à 113)
    }
    // M7 (L3-C9) : un trait en diagonale ou en L se désigne par son encre. Chaque point refusé est dans son
    // emprise (boundsOnPage) mais loin de sa ligne : une désignation par l'emprise le prendrait.
    void strokeHitFollowsTheInkNotTheBounds()
    {
        Page diagonal;
        insertAtTopOfLayer(diagonal, strokeObject(7, {{0, 0, 0.5f}, {400, 400, 0.5f}}, 10));
        QVERIFY(boundsOnPage(diagonal.objects[0]).contains({300, 100}));
        QVERIFY(!objectAt(diagonal, {300, 100}, 8).has_value());              // à 141 de la ligne
        QCOMPARE(objectAt(diagonal, {200, 203}, 0).value_or(0), ObjectId{7}); // à 2,1 de la ligne
        Page corner;
        insertAtTopOfLayer(corner, strokeObject(8, {{0, 0, 0.5f}, {400, 0, 0.5f}, {400, 400, 0.5f}}, 10));
        QVERIFY(boundsOnPage(corner.objects[0]).contains({100, 300}));
        QVERIFY(!objectAt(corner, {100, 300}, 8).has_value());              // à 300 de chaque branche
        QCOMPARE(objectAt(corner, {397, 300}, 0).value_or(0), ObjectId{8}); // sur la branche verticale
    }
    // M4 (L3-C9) : un groupe et un objet inconnu sans boîte n'ont pas d'emprise (boundsOnPage rend 0 × 0 à
    // l'origine) et ne se dessinent pas : ni un clic près de l'origine ni un rectangle qui la touche ne les
    // désignent. Un objet inconnu qui a une boîte se désigne, lui.
    void objectsWithoutBoundsAreNotDesignated()
    {
        Page page;
        insertAtTopOfLayer(page, rectObject(1, {100, 100, 50, 50}));
        Group group;
        group.members = {1};
        PageObject grouped{group};
        grouped.common.id = 2;
        insertAtTopOfLayer(page, grouped);
        PageObject unknown{UnknownObject{}};
        unknown.common.id = 3;
        insertAtTopOfLayer(page, unknown);
        UnknownObject boxed;
        boxed.bounds = {300, 300, 40, 40};
        boxed.hasBounds = true;
        PageObject withBox{boxed};
        withBox.common.id = 4;
        insertAtTopOfLayer(page, withBox);

        QVERIFY(!hasExtent(grouped));
        QVERIFY(!hasExtent(unknown));
        QVERIFY(hasExtent(withBox));
        QVERIFY(!isSelectable(grouped));
        QVERIFY(!isSelectable(unknown));
        QVERIFY(isSelectable(withBox));
        QVERIFY(!isSelectable(strokeObject(5, {}, 4)));  // un trait sans point n'a pas d'emprise non plus
        QVERIFY(!objectAt(page, {2, 2}, 6).has_value()); // à moins de 6 px de l'origine
        QCOMPARE(objectsTouching(page, normalizedRect({-20, -20}, {120, 120})), (std::vector<ObjectId>{1}));
        QCOMPARE(objectAt(page, {320, 320}, 0).value_or(0), ObjectId{4});
        QCOMPARE(objectsTouching(page, normalizedRect({-20, -20}, {500, 500})),
                 (std::vector<ObjectId>{1, 4}));
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
    // m5 (L3-C7) : un trait agrandi ×3 se désigne dans toute son épaisseur visible, pas dans le tiers.
    void scaledStrokeHitUsesTheVisibleWidth()
    {
        Page page;
        PageObject o = strokeObject(9, {{100, 100, 0.5f}, {300, 100, 0.5f}}, 10); // demi-largeur locale : 5
        o.common.transform = scalingAbout({200, 100}, 3, 3);                      // visible : 15
        insertAtTopOfLayer(page, o);
        QCOMPARE(objectAt(page, {200, 107}, 0).value_or(0), ObjectId{9}); // 1,4 × 5 sous la ligne
        QVERIFY(!objectAt(page, {200, 116}, 0).has_value());              // au-delà de 15
    }
};

QTEST_GUILESS_MAIN(TstCoreHittest)
#include "tst_core_hittest.moc"
