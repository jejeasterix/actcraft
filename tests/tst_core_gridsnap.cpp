#include <actcraft/core/gridsnap.h>

#include <QtTest>

#include <algorithm>
#include <cmath>
#include <limits>

using namespace actcraft::core;

namespace {
constexpr double kPi = 3.14159265358979323846;

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

// Un niveau incliné (famille + au pas X, famille − au pas Y), début `start`.
Grid tilted(GridPattern pattern, double angle, double pitchX, double pitchY, PointF start = {})
{
    Grid g = grid(pitchX);
    g.levels[0].pattern = pattern;
    g.levels[0].angle = angle;
    g.levels[0].spacingY = pitchY;
    g.levels[0].start = start;
    return g;
}

bool sameSpot(PointF a, PointF b)
{
    return std::abs(a.x - b.x) < 1e-6 && std::abs(a.y - b.y) < 1e-6;
}

double squared(PointF a, PointF b)
{
    return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
}

double cotangentOf(const GridLevel& l)
{
    const double degrees = l.pattern == GridPattern::Diagonal45 ? 45.0 : l.angle;
    return 1.0 / std::tan(degrees * kPi / 180.0);
}

// Oracle lent, réservé au test : plus petit carré de distance entre p et un croisement des deux familles
// inclinées tombant dans le carré [p − r, p + r], énumérés ligne à ligne avec la géométrie du peintre
// (famille + x = a + t, famille − x = b − t, t = (y − début Y)·cot). Infini s'il n'y en a aucun. Pas de
// plafond : exact pour ce carré.
double slowNearestSquared(const GridLevel& l, PointF p, double r)
{
    const double cot = cotangentOf(l);
    const double xLo = p.x - r;
    const double xHi = p.x + r;
    const double tLo = (p.y - r - l.start.y) * cot;
    const double tHi = (p.y + r - l.start.y) * cot;
    double best = std::numeric_limits<double>::infinity();
    // Croisement : t = (b − a) / 2, x = a + t dans [xLo, xHi], t dans [tLo, tHi].
    const auto iFirst = static_cast<long long>(std::ceil((xLo - tHi - l.start.x) / l.spacingX));
    const auto iLast = static_cast<long long>(std::floor((xHi - tLo - l.start.x) / l.spacingX));
    for (long long i = iFirst; i <= iLast; ++i) {
        const double a = l.start.x + static_cast<double>(i) * l.spacingX;
        const double t0 = std::max(tLo, xLo - a);
        const double t1 = std::min(tHi, xHi - a);
        if (t0 > t1) {
            continue;
        }
        const auto jFirst = static_cast<long long>(std::ceil((a + 2 * t0 - l.start.x) / l.spacingY));
        const auto jLast = static_cast<long long>(std::floor((a + 2 * t1 - l.start.x) / l.spacingY));
        for (long long j = jFirst; j <= jLast; ++j) {
            const double b = l.start.x + static_cast<double>(j) * l.spacingY;
            const double t = (b - a) / 2;
            best = std::min(best, squared({a + t, l.start.y + t / cot}, p));
        }
    }
    return best;
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
    void steepGridFindsTheNearestRow()
    {
        // 89°, pas 20 : les croisements forment des rangées horizontales espacées de (pas / 2) × tan 89° ≈
        // 573. Entre deux rangées, le croisement le plus proche est loin du point, hors d'un voisinage de
        // quelques pas.
        const auto p = nearestGridPoint(tilted(GridPattern::Angle, 89, 20, 20), {105, 100}, 1024, 768);
        QVERIFY(p.has_value());
        QVERIFY2(sameSpot(*p, {100, 0}), qPrintable(QStringLiteral("(%1, %2)").arg(p->x).arg(p->y)));
    }
    void steepGridKeepsTheNearestCrossingOfItsRow()
    {
        // Arrondir chaque indice de famille séparément donnerait le croisement (110, 573) ; le plus proche
        // est (120, 0) sur la même rangée, à 14 de distance.
        const auto p = nearestGridPoint(tilted(GridPattern::Angle, 89, 20, 20), {110.1, 10}, 1024, 768);
        QVERIFY(p.has_value());
        QVERIFY2(sameSpot(*p, {120, 0}), qPrintable(QStringLiteral("(%1, %2)").arg(p->x).arg(p->y)));
    }
    void unevenStepsFindTheNearbyCrossing()
    {
        // 45°, pas 1 × 4096 : croisements x = (a + b) / 2, y = (b − a) / 2, a entier, b multiple de 4096.
        const auto p =
            nearestGridPoint(tilted(GridPattern::Diagonal45, 45, 1, 4096), {2048.3, 2047.6}, 1024, 768);
        QVERIFY(p.has_value());
        QVERIFY2(sameSpot(*p, {2048.5, 2047.5}), qPrintable(QStringLiteral("(%1, %2)").arg(p->x).arg(p->y)));
    }
    void tiltedSnapIsTheNearestCrossing()
    {
        // Pour chaque angle, paire de pas et point : le résultat est un croisement (indices entiers sur les
        // deux familles) et l'oracle lent ne trouve aucun croisement plus proche dans le carré de demi-côté d
        // + 1.
        const double angles[] = {1, 5, 30, 45, 60, 85, 89};
        const double steps[][2] = {{20, 20}, {20, 35}, {1, 1000}, {7, 3}};
        for (const double angle : angles) {
            for (const auto& s : steps) {
                for (int k = 0; k < 49; ++k) {
                    const PointF start = (k % 2 == 1) ? PointF{13, -7} : PointF{};
                    const Grid g = tilted(GridPattern::Angle, angle, s[0], s[1], start);
                    const PointF p{static_cast<double>(k % 7) * 47.3 + 3.1,
                                   static_cast<double>(k / 7) * 41.9 + 2.7};
                    const QString where = QStringLiteral("angle %1, pas %2 × %3, point (%4, %5)")
                                              .arg(angle)
                                              .arg(s[0])
                                              .arg(s[1])
                                              .arg(p.x)
                                              .arg(p.y);
                    const auto q = nearestGridPoint(g, p, 1024, 768);
                    QVERIFY2(q.has_value(), qPrintable(where));
                    const GridLevel& l = g.levels[0];
                    const double cot = cotangentOf(l);
                    const double i = (q->x - l.start.x - (q->y - l.start.y) * cot) / l.spacingX;
                    const double j = (q->x - l.start.x + (q->y - l.start.y) * cot) / l.spacingY;
                    QVERIFY2(std::abs(i - std::nearbyint(i)) < 1e-6 && std::abs(j - std::nearbyint(j)) < 1e-6,
                             qPrintable(where));
                    const double d2 = squared(*q, p);
                    QVERIFY2(slowNearestSquared(l, p, std::sqrt(d2) + 1) >= d2 - 1e-6 * (1 + d2),
                             qPrintable(where));
                }
            }
        }
    }
    void diagonalSnapsOntoItsOnlyFamily()
    {
        // Le peintre ne trace la famille + que si linesAlongX, la famille − que si linesAlongY. Une seule
        // famille : point le plus proche sur la ligne la plus proche, comme l'axe non dessiné du niveau
        // normal.
        Grid g = tilted(GridPattern::Diagonal45, 45, 50, 50);
        const auto both = nearestGridPoint(g, {30, 10}, 1024, 768);
        QVERIFY(both.has_value());
        QVERIFY(sameSpot(*both, {25, 25}));
        g.levels[0].linesAlongY = false; // famille + seule : x = 50·i + y
        const auto plus = nearestGridPoint(g, {30, 10}, 1024, 768);
        QVERIFY(plus.has_value());
        QVERIFY2(sameSpot(*plus, {20, 20}), qPrintable(QStringLiteral("(%1, %2)").arg(plus->x).arg(plus->y)));
        g.levels[0].linesAlongY = true;
        g.levels[0].linesAlongX = false; // famille − seule : x = 50·j − y
        const auto minus = nearestGridPoint(g, {30, 10}, 1024, 768);
        QVERIFY(minus.has_value());
        QVERIFY2(sameSpot(*minus, {35, 15}),
                 qPrintable(QStringLiteral("(%1, %2)").arg(minus->x).arg(minus->y)));
    }
    void levelWithoutLinesDoesNotSnap()
    {
        Grid g = grid(32);
        g.levels[0].linesAlongX = false;
        g.levels[0].linesAlongY = false;
        QVERIFY(!nearestGridPoint(g, {74, 26}, 1024, 768).has_value());
        // Un niveau sans ligne ne masque pas le niveau suivant.
        GridLevel coarse;
        coarse.spacingX = coarse.spacingY = 50;
        g.levels.push_back(coarse);
        const auto p = nearestGridPoint(g, {74, 26}, 1024, 768);
        QVERIFY(p.has_value());
        QCOMPARE(p->x, 50.0);
        QCOMPARE(p->y, 50.0);
        // Niveau incliné sans aucune famille.
        Grid diagonal = tilted(GridPattern::Angle, 30, 20, 20);
        diagonal.levels[0].linesAlongX = false;
        diagonal.levels[0].linesAlongY = false;
        QVERIFY(!nearestGridPoint(diagonal, {74, 26}, 1024, 768).has_value());
    }
    void normalLevelSnapsFromItsStart()
    {
        // Le peintre trace les lignes du niveau normal à partir de max(0, début) : pas d'accrochage en deçà.
        Grid g = grid(50);
        g.levels[0].start = {100, 100};
        const auto p = nearestGridPoint(g, {20, 30}, 1024, 768);
        QVERIFY(p.has_value());
        QCOMPARE(p->x, 100.0);
        QCOMPARE(p->y, 100.0);
        // Début négatif : première ligne tracée en 20 (−30 + 50), pas en −30.
        g.levels[0].start = {-30, -30};
        const auto q = nearestGridPoint(g, {-5, 1}, 1024, 768);
        QVERIFY(q.has_value());
        QCOMPARE(q->x, 20.0);
        QCOMPARE(q->y, 20.0);
        // Dans la zone tracée, rien ne change.
        g.levels[0].start = {100, 100};
        const auto r = nearestGridPoint(g, {174, 126}, 1024, 768);
        QVERIFY(r.has_value());
        QCOMPARE(r->x, 150.0);
        QCOMPARE(r->y, 150.0);
    }
};

QTEST_GUILESS_MAIN(TstCoreGridsnap)
#include "tst_core_gridsnap.moc"
