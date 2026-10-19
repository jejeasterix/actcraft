#include <actcraft/core/erasegeometry.h>

#include <QTest>

using namespace actcraft::core;

namespace {
std::vector<StrokePoint> line(double y = 100)
{
    return {{0, y, 0.2f}, {100, y, 0.4f}, {200, y, 0.6f}, {300, y, 0.8f}};
}
} // namespace

class TstCoreErase : public QObject
{
    Q_OBJECT

private slots:
    void middleCutSplitsInTwo()
    {
        const StrokeCut cut = cutByDisk(line(), {150, 100}, 20);
        QVERIFY(cut.touched);
        QCOMPARE(cut.pieces.size(), std::size_t{2});
        QCOMPARE(cut.pieces[0].back().x, 130.0);
        QCOMPARE(cut.pieces[1].front().x, 170.0);
        QVERIFY(qAbs(cut.pieces[0].back().pressure - 0.46f) <
                1e-4f); // interpolée entre 0.4 (x=100) et 0.6 (x=200) en x=130
    }
    void endCutShortens()
    {
        const StrokeCut cut = cutByDisk(line(), {300, 100}, 50);
        QVERIFY(cut.touched);
        QCOMPARE(cut.pieces.size(), std::size_t{1});
        QCOMPARE(cut.pieces[0].back().x, 250.0);
    }
    void missLeavesIntact()
    {
        const StrokeCut cut = cutByDisk(line(), {150, 200}, 20);
        QVERIFY(!cut.touched);
        QCOMPARE(cut.pieces.size(), std::size_t{1});
        QCOMPARE(cut.pieces[0].size(), std::size_t{4});
    }
    void wholeStrokeInsideIsRemoved()
    {
        const StrokeCut cut = cutByDisk({{10, 10, 0.5f}, {12, 10, 0.5f}}, {11, 10}, 20);
        QVERIFY(cut.touched);
        QVERIFY(cut.pieces.empty());
    }
    void sweepDoesNotSkip()
    {
        // Deux positions de gomme éloignées de part et d'autre d'un trait vertical : le balayage le coupe.
        const std::vector<StrokePoint> vertical = {{150, 0, 0.5f}, {150, 300, 0.5f}};
        QVERIFY(!cutByDisk(vertical, {100, 150}, 10).touched);
        QVERIFY(!cutByDisk(vertical, {200, 150}, 10).touched);
        const StrokeCut cut = cutBySweep(vertical, {100, 150}, {200, 150}, 10);
        QVERIFY(cut.touched);
        QCOMPARE(cut.pieces.size(), std::size_t{2});
    }
    void pointsOnPageApplyTheTransform()
    {
        Stroke s;
        s.points = {{0, 0, 0.5f}, {10, 0, 0.5f}};
        PageObject o{s};
        o.common.transform = translation(100, 50);
        const auto pts = strokePointsOnPage(o);
        QCOMPARE(pts.size(), std::size_t{2});
        QCOMPARE(pts[1].x, 110.0);
        QCOMPARE(pts[1].y, 50.0);
    }
};

QTEST_GUILESS_MAIN(TstCoreErase)
#include "tst_core_erase.moc"
