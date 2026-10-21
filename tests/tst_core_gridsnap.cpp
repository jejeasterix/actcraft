#include <actcraft/core/gridsnap.h>

#include <QtTest>

#include <cmath>

using namespace actcraft::core;

namespace {
Grid grid(double step, bool snapEnabled = true)
{
    Grid g;
    g.visible = true;
    g.snapEnabled = snapEnabled;
    GridLevel level;
    level.spacingX = step;
    level.spacingY = step;
    g.levels.push_back(level);
    return g;
}
} // namespace

class TstCoreGridsnap : public QObject
{
    Q_OBJECT

private slots:
    void nearestOnNormalGrid()
    {
        const auto p = nearestGridPoint(grid(50), {74, 26}, 1024, 768);
        QVERIFY(p.has_value());
        QCOMPARE(p->x, 50.0);
        QCOMPARE(p->y, 50.0);
        // « Échelle par défaut » : pas effectif = pas × baseZoom.
        Grid zoomed = grid(50);
        zoomed.baseZoom = 2;
        const auto q = nearestGridPoint(zoomed, {74, 26}, 1024, 768);
        QVERIFY(q.has_value());
        QCOMPARE(q->x, 100.0);
        QCOMPARE(q->y, 0.0);
    }
    void onlySnappingLevelsCount()
    {
        Grid g = grid(50);
        GridLevel fine;
        fine.spacingX = fine.spacingY = 10;
        fine.snap = false;
        g.levels.push_back(fine);
        QCOMPARE(nearestGridPoint(g, {74, 26}, 1024, 768)->x, 50.0);
    }
    void invisibleGridStillSnaps()
    {
        Grid g = grid(50);
        g.visible = false;
        QVERIFY(nearestGridPoint(g, {74, 26}, 1024, 768).has_value());
    }
    void snapDisabledReturnsNothing()
    {
        QVERIFY(!nearestGridPoint(grid(50, false), {74, 26}, 1024, 768).has_value());
    }
    void bottomLeftCornerLandsOnCrossing()
    {
        Page page;
        page.grid = grid(50);
        PageObject o{Shape{}};
        o.common.id = 7;
        std::get<Shape>(body(o)).bounds = {0, 0, 40, 30};
        o.common.transform = translation(12, 12); // coin bas gauche en (12, 42)
        page.objects.push_back(o);
        const PointF d = snapDelta(page, {7}, {30, 0}); // ancrage visé (42, 42) → (50, 50)
        QCOMPARE(d.x, 38.0);
        QCOMPARE(d.y, 8.0);
        page.grid.snapEnabled = false;
        QCOMPARE(snapDelta(page, {7}, {30, 0}).x, 30.0);
    }
    void textUsesItsBaseline()
    {
        PageObject o{Text{}};
        auto& text = std::get<Text>(body(o));
        text.bounds = {0, 0, 100, 40};
        TextSpan span;
        span.baseline = {0, 30};
        text.runs.push_back(span);
        o.common.transform = translation(10, 10);
        const PointF a = snapAnchor(o);
        QCOMPARE(a.x, 10.0);
        QCOMPARE(a.y, 40.0);
    }
    void diagonalGridCrossings()
    {
        Grid g = grid(50);
        g.levels[0].pattern = GridPattern::Diagonal45;
        const auto p = nearestGridPoint(g, {26, 24}, 1024, 768);
        QVERIFY(p.has_value());
        QVERIFY(std::hypot(p->x - 26, p->y - 24) <= 50 * std::sqrt(2.0));
        // Croisement exact des deux familles à 45° : (25, 25).
        QCOMPARE(p->x, 25.0);
        QCOMPARE(p->y, 25.0);
    }
};

QTEST_GUILESS_MAIN(TstCoreGridsnap)
#include "tst_core_gridsnap.moc"
