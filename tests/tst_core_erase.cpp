#include <actcraft/core/erasegeometry.h>

#include <QTest>

using namespace actcraft::core;

namespace {
std::vector<StrokePoint> line(double y = 100)
{
    return {{0, y, 0.2f}, {100, y, 0.4f}, {200, y, 0.6f}, {300, y, 0.8f}};
}

// Constat de S6 (L3-C6) : trait y = 300 de x = 200 à 600 en 16 sous-segments de 25 px, sommets entiers ; un
// disque de rayon 25 centré en (400, 300) met les sommets 375 et 425 pile sur le cercle.
std::vector<StrokePoint> sixteenSegments()
{
    std::vector<StrokePoint> out;
    for (int x = 200; x <= 600; x += 25)
        out.push_back({static_cast<double>(x), 300, 0.5f});
    return out;
}

bool near(double a, double b)
{
    return qAbs(a - b) < 1e-9;
}

// Les deux morceaux attendus de part et d'autre du disque (400, 300, r 25) : [200, 375] et [425, 600].
bool twoPiecesAroundTheDisk(const std::vector<std::vector<StrokePoint>>& pieces)
{
    return pieces.size() == 2 && near(pieces[0].front().x, 200) && near(pieces[0].back().x, 375) &&
           near(pieces[1].front().x, 425) && near(pieces[1].back().x, 600);
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
        // Geste immobile : un seul centre. Le morceau dont le bout tombe pile sur le cercle ne doit pas être
        // mangé par une seconde application du même centre (le bord comptait dedans avant L3-C6 ; commentaire
        // seul mis à jour, assertions inchangées).
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
    // L3-C6 : bord du disque compté dehors, pour les sommets comme pour les croisements.
    void verticesOnTheCircleKeepBothSides()
    {
        const StrokeCut cut = cutByDisk(sixteenSegments(), {400, 300}, 25);
        QVERIFY(cut.touched);
        QCOMPARE(cut.pieces.size(), std::size_t{2});
        QVERIFY2(twoPiecesAroundTheDisk(cut.pieces), "morceaux attendus : [200, 375] et [425, 600]");
        QCOMPARE(cut.pieces[0].size(), std::size_t{8}); // 200 … 375, sans point de coupe en double
        QCOMPARE(cut.pieces[1].size(), std::size_t{8}); // 425 … 600
    }
    void chainedSweepThroughVerticesOnTheCircle()
    {
        // Comme l'outil : un balayage par mouvement de 10 px de (400, 260) à (400, 340), le premier avec son
        // centre de départ, les suivants sans ; le centre (400, 300) est appliqué tel quel.
        std::vector<std::vector<StrokePoint>> pieces{sixteenSegments()};
        bool touched = false;
        for (int y = 260; y < 340; y += 10) {
            std::vector<std::vector<StrokePoint>> next;
            for (const auto& piece : pieces) {
                const StrokeCut cut = cutBySweep(piece, {400, static_cast<double>(y)},
                                                 {400, static_cast<double>(y + 10)}, 25, y == 260);
                touched = touched || cut.touched;
                for (const auto& q : cut.pieces)
                    next.push_back(q);
            }
            pieces = std::move(next);
        }
        QVERIFY(touched);
        QCOMPARE(pieces.size(), std::size_t{2});
        QVERIFY2(twoPiecesAroundTheDisk(pieces), "morceaux attendus : [200, 375] et [425, 600]");
    }
    void tangentSegmentIsNotCut()
    {
        // Segment tangent en (400, 275) : contact sans longueur, rien n'est retiré (disc == 0 en entiers).
        const std::vector<StrokePoint> tangent = {{300, 275, 0.5f}, {500, 275, 0.5f}};
        const StrokeCut plain = cutByDisk(tangent, {400, 300}, 25);
        QVERIFY(!plain.touched);
        QCOMPARE(plain.pieces.size(), std::size_t{1});
        QCOMPARE(plain.pieces[0].size(), std::size_t{2});
        // Même tangente avec un sommet au point de contact : le sommet reste, le trait n'est pas touché.
        const std::vector<StrokePoint> withVertex = {{300, 275, 0.5f}, {400, 275, 0.5f}, {500, 275, 0.5f}};
        const StrokeCut atVertex = cutByDisk(withVertex, {400, 300}, 25);
        QVERIFY(!atVertex.touched);
        QCOMPARE(atVertex.pieces.size(), std::size_t{1});
        QCOMPARE(atVertex.pieces[0].size(), std::size_t{3});
        QCOMPARE(atVertex.pieces[0][1].x, 400.0);
    }
    void strokeEndOnTheCircleIsKept()
    {
        // Extrémité pile sur le cercle, le reste dehors : le trait reste entier, dans les deux sens.
        const std::vector<StrokePoint> endsOnIt = {{200, 300, 0.5f}, {375, 300, 0.5f}};
        const StrokeCut end = cutByDisk(endsOnIt, {400, 300}, 25);
        QVERIFY(!end.touched);
        QCOMPARE(end.pieces.size(), std::size_t{1});
        QCOMPARE(end.pieces[0].size(), std::size_t{2});
        QCOMPARE(end.pieces[0].back().x, 375.0);
        const std::vector<StrokePoint> startsOnIt = {{375, 300, 0.5f}, {200, 300, 0.5f}};
        const StrokeCut start = cutByDisk(startsOnIt, {400, 300}, 25);
        QVERIFY(!start.touched);
        QCOMPARE(start.pieces.size(), std::size_t{1});
        QCOMPARE(start.pieces[0].size(), std::size_t{2});
        QCOMPARE(start.pieces[0].front().x, 375.0);
    }
    void insideVertexNextToAVertexOnTheCircle()
    {
        // Sommet intérieur (le centre) suivi d'un sommet sur le bord : le morceau repart du sommet du bord.
        const std::vector<StrokePoint> leaving = {
            {400, 300, 0.5f}, {425, 300, 0.5f}, {450, 300, 0.5f}, {500, 300, 0.5f}};
        const StrokeCut out = cutByDisk(leaving, {400, 300}, 25);
        QVERIFY(out.touched);
        QCOMPARE(out.pieces.size(), std::size_t{1});
        QCOMPARE(out.pieces[0].size(), std::size_t{3});
        QCOMPARE(out.pieces[0].front().x, 425.0);
        // Sens inverse : sommet sur le bord suivi d'un sommet intérieur ; le morceau finit au sommet du bord.
        const std::vector<StrokePoint> entering = {{350, 300, 0.5f}, {375, 300, 0.5f}, {400, 300, 0.5f}};
        const StrokeCut in = cutByDisk(entering, {400, 300}, 25);
        QVERIFY(in.touched);
        QCOMPARE(in.pieces.size(), std::size_t{1});
        QCOMPARE(in.pieces[0].size(), std::size_t{2});
        QCOMPARE(in.pieces[0].back().x, 375.0);
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
