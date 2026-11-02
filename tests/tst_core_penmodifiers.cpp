#include <actcraft/core/penmodifiers.h>

#include <QTest>

#include <algorithm>
#include <cmath>

using actcraft::core::PointF;

namespace {
bool same(const PointF& a, const PointF& b)
{
    return std::abs(a.x - b.x) < 1e-9 && std::abs(a.y - b.y) < 1e-9;
}
} // namespace

class TestPenModifiers : public QObject
{
    Q_OBJECT
private slots:
    void horizontalLineIgnoresVerticalMotion()
    {
        const auto p = actcraft::core::penShapePoints(actcraft::core::PenShape::Horizontal, {10, 10}, {200, 90});
        QVERIFY(!p.empty());
        QCOMPARE(p.front().y, 10.0);
        QVERIFY(same(p.back(), {200, 10}));
    }
    void verticalLineIgnoresHorizontalMotion()
    {
        const auto p = actcraft::core::penShapePoints(actcraft::core::PenShape::Vertical, {0, 0}, {100, 40});
        QVERIFY(!p.empty());
        QVERIFY(same(p.back(), {0, 40}));
    }
    void squareHasEqualSides()
    {
        const auto p = actcraft::core::penShapePoints(actcraft::core::PenShape::Square, {0, 0}, {100, 40});
        QCOMPARE(p.size(), std::size_t(5)); // fermé
        QVERIFY(same(p[2], {100, 100}));    // côté = la plus grande des deux dimensions
        QVERIFY(same(p.front(), p.back()));
    }
    void rectangleIsClosedWithOppositeCorner()
    {
        const auto p = actcraft::core::penShapePoints(actcraft::core::PenShape::Rectangle, {0, 0}, {100, 40});
        QCOMPARE(p.size(), std::size_t(5));
        QVERIFY(same(p[2], {100, 40}));
        QVERIFY(same(p.front(), p.back()));
    }
    void circleClosed()
    {
        const auto p = actcraft::core::penShapePoints(actcraft::core::PenShape::Circle, {0, 0}, {100, 100});
        QVERIFY(p.size() >= 32);
        QVERIFY(same(p.front(), p.back()));
    }
    void ellipseIsClosedAndFillsTheBox()
    {
        const auto p = actcraft::core::penShapePoints(actcraft::core::PenShape::Ellipse, {0, 0}, {100, 40});
        QVERIFY(p.size() >= 32);
        QVERIFY(same(p.front(), p.back()));
        const auto maxX = std::max_element(p.begin(), p.end(), [](auto& a, auto& b) { return a.x < b.x; })->x;
        QVERIFY(std::abs(maxX - 100.0) < 1e-9);
    }
    void arcIsAHalfEllipse()
    {
        const auto p = actcraft::core::penShapePoints(actcraft::core::PenShape::Arc, {0, 0}, {100, 40});
        QVERIFY(p.size() >= 16);
        QVERIFY(same(p.front(), {0, 20}));
        QVERIFY(same(p.back(), {100, 20}));
        const auto minY = std::min_element(p.begin(), p.end(), [](auto& a, auto& b) { return a.y < b.y; })->y;
        QVERIFY(std::abs(minY - 0.0) < 1e-9);
    }
    void polylineGoesThroughTheClicks()
    {
        const auto p =
            actcraft::core::penShapePoints(actcraft::core::PenShape::Polyline, {0, 0}, {100, 40}, {{50, 0}});
        QCOMPARE(p.size(), std::size_t(3));
        QVERIFY(same(p[0], {0, 0}));
        QVERIFY(same(p[1], {50, 0}));
        QVERIFY(same(p[2], {100, 40}));
    }
    void arrowHasAHead()
    {
        const auto p = actcraft::core::penShapePoints(actcraft::core::PenShape::Arrow, {0, 0}, {100, 0});
        QVERIFY(p.size() >= 5); // fût, puis deux branches de la pointe en un seul tracé
        QVERIFY(same(p.front(), {0, 0}));
        QVERIFY(same(p[1], {100, 0}));
        QVERIFY(std::abs(std::hypot(p[2].x - 100, p[2].y) - 20.0) < 1e-9); // pointe de 20 unités
    }
    void zigzagStepsBy16()
    {
        const auto p = actcraft::core::penShapePoints(actcraft::core::PenShape::Zigzag, {0, 0}, {100, 40});
        QVERIFY(p.size() >= 4);
        QVERIFY(same(p.front(), {0, 0}));
        QVERIFY(same(p.back(), {100, p.back().y}));
        for (std::size_t i = 1; i < p.size(); ++i) {
            QVERIFY(p[i].x > p[i - 1].x);
            QVERIFY(p[i].x - p[i - 1].x <= 16.0 + 1e-9);
        }
        QVERIFY(std::abs(p[1].x - 16.0) < 1e-9);
        QVERIFY(p[1].y != p[0].y); // le tracé alterne
    }
    void freeKeepsTheTwoPoints()
    {
        const auto p = actcraft::core::penShapePoints(actcraft::core::PenShape::Free, {0, 0}, {100, 40});
        QCOMPARE(p.size(), std::size_t(2));
        QVERIFY(same(p[0], {0, 0}));
        QVERIFY(same(p[1], {100, 40}));
    }
    void resultIsAStrokeOnTheTopLayer()
    {
        actcraft::core::Outline pen;
        pen.color = 0xFF0000FFu;
        pen.width = 4;
        const auto object = actcraft::core::penShapeStroke({{0, 0}, {10, 0}}, pen, false);
        const auto* stroke = std::get_if<actcraft::core::Stroke>(&object);
        QVERIFY(stroke);
        QCOMPARE(object.common.layer, actcraft::core::Layer::Top);
        QCOMPARE(stroke->points.size(), std::size_t(2));
        QCOMPARE(stroke->color, 0xFF0000FFu);
        QCOMPARE(stroke->baseWidth, 4.0);
        QVERIFY(stroke->kind == actcraft::core::StrokeKind::Pen);
        QVERIFY(actcraft::core::penShapeFromId("zigzag") == std::optional(actcraft::core::PenShape::Zigzag));
        QVERIFY(!actcraft::core::penShapeFromId("autre"));
    }
    void highlighterKeepsItsKind()
    {
        const auto object = actcraft::core::penShapeStroke({{0, 0}, {10, 0}}, actcraft::core::Outline{}, true);
        const auto* stroke = std::get_if<actcraft::core::Stroke>(&object);
        QVERIFY(stroke);
        QVERIFY(stroke->kind == actcraft::core::StrokeKind::Highlighter);
        QCOMPARE(object.common.layer, actcraft::core::Layer::Top);
    }
};

QTEST_GUILESS_MAIN(TestPenModifiers)
#include "tst_core_penmodifiers.moc"
