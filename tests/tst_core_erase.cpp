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
    void sweepNeverReappliesACenter()
    {
        // Geste immobile : un seul centre. Le morceau dont le bout tombe pile sur le cercle (`<=` :
        // intérieur) ne doit pas être mangé par une seconde application du même centre.
        const std::vector<StrokePoint> segment = {{0, 100, 0.5f}, {300, 100, 0.5f}};
        const StrokeCut still = cutBySweep(segment, {150, 100}, {150, 100}, 20);
        QVERIFY(still.touched);
        QCOMPARE(still.pieces.size(), std::size_t{2});
        QCOMPARE(still.pieces[0].back().x, 130.0);
        QCOMPARE(still.pieces[1].front().x, 170.0);

        // Enchaînement a -> b puis b -> c (includeFrom = false au second appel) : mêmes centres, donc même
        // résultat qu'un seul balayage a -> c (pas de 10 dans les deux cas).
        const PointF a{60, 100};
        const PointF b{80, 100};
        const PointF c{100, 100};
        const StrokeCut whole = cutBySweep(segment, a, c, 20);
        const StrokeCut first = cutBySweep(segment, a, b, 20);
        std::vector<std::vector<StrokePoint>> chained;
        bool touched = first.touched;
        for (const auto& piece : first.pieces) {
            const StrokeCut next = cutBySweep(piece, b, c, 20, false);
            touched = touched || next.touched;
            for (const auto& q : next.pieces)
                chained.push_back(q);
        }
        QCOMPARE(touched, whole.touched);
        QCOMPARE(chained.size(), whole.pieces.size());
        for (std::size_t i = 0; i < chained.size(); ++i) {
            QCOMPARE(chained[i].size(), whole.pieces[i].size());
            for (std::size_t k = 0; k < chained[i].size(); ++k)
                QCOMPARE(chained[i][k].x, whole.pieces[i][k].x);
        }
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
