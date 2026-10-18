#include <actcraft/features/shapefinder.h>

#include <QRandomGenerator>
#include <QtTest>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

using namespace actcraft::features;

namespace {
std::vector<QPointF> noisy(std::vector<QPointF> pts, double amplitude, quint32 seed)
{
    QRandomGenerator g(seed);
    for (auto& p : pts) {
        // Deux instructions : x puis y, quel que soit le compilateur (l'ordre des arguments n'est pas fixé).
        const double dx = (g.generateDouble() * 2 - 1) * amplitude;
        const double dy = (g.generateDouble() * 2 - 1) * amplitude;
        p += QPointF(dx, dy);
    }
    return pts;
}
std::vector<QPointF> ellipse(QPointF c, double rx, double ry, double tiltDeg, int n, double turns = 1.0)
{
    std::vector<QPointF> out;
    const double t0 = qDegreesToRadians(tiltDeg);
    for (int i = 0; i <= n; ++i) {
        const double a = 2 * std::numbers::pi * turns * i / n;
        const double x = rx * std::cos(a), y = ry * std::sin(a);
        out.emplace_back(c.x() + x * std::cos(t0) - y * std::sin(t0),
                         c.y() + x * std::sin(t0) + y * std::cos(t0));
    }
    return out;
}
std::vector<QPointF> polyline(const std::vector<QPointF>& corners, int perSide)
{
    std::vector<QPointF> out;
    for (std::size_t i = 0; i < corners.size(); ++i) {
        const QPointF a = corners[i], b = corners[(i + 1) % corners.size()];
        for (int k = 0; k < perSide; ++k)
            out.push_back(a + (b - a) * (static_cast<double>(k) / perSide));
    }
    out.push_back(corners.front());
    return out;
}
double distanceToNearest(QPointF p, const std::vector<QPointF>& points)
{
    double best = std::numeric_limits<double>::max();
    for (const QPointF& q : points)
        best = std::min(best, std::hypot(p.x() - q.x(), p.y() - q.y()));
    return best;
}
// Coins rendus et coins tracés se répondent un à un, à moins de 8 pixels (bruit de ± 3 pixels, sommets
// ajustés).
bool sameCorners(const std::vector<QPointF>& got, const std::vector<QPointF>& drawn)
{
    constexpr double tolerance = 8;
    if (got.size() != drawn.size())
        return false;
    return std::all_of(got.begin(), got.end(),
                       [&](QPointF p) { return distanceToNearest(p, drawn) < tolerance; }) &&
           std::all_of(drawn.begin(), drawn.end(),
                       [&](QPointF p) { return distanceToNearest(p, got) < tolerance; });
}
} // namespace

class TstFeaturesShapeFinder : public QObject
{
    Q_OBJECT
private slots:
    void straightLineIsALine()
    {
        const auto r = recognizeShape(noisy({{100, 100}, {200, 150}, {300, 200}, {400, 250}}, 2, 1));
        QVERIFY(r.has_value());
        QCOMPARE(r->kind, RecognizedShape::Kind::Line);
    }
    void wobblyCircleIsACircle()
    {
        const auto r = recognizeShape(noisy(ellipse({300, 300}, 100, 100, 0, 80), 3, 2));
        QVERIFY(r.has_value());
        QCOMPARE(r->kind, RecognizedShape::Kind::Circle);
        QVERIFY(std::abs(r->bounds.width() - 200) < 12);
    }
    void tiltedEllipseKeepsItsAngle()
    {
        const auto r = recognizeShape(noisy(ellipse({300, 300}, 150, 70, 30, 90), 2, 3));
        QVERIFY(r.has_value());
        QCOMPARE(r->kind, RecognizedShape::Kind::Ellipse);
        QVERIFY(std::abs(r->angleDegrees - 30) < 6);
        // Inclinée à 45° : sa boîte englobante est carrée. Classer par la boîte et le rapport de ses côtés en
        // ferait un cercle d'angle 0 ; les axes principaux disent ellipse, 45°.
        const auto r45 = recognizeShape(noisy(ellipse({300, 300}, 150, 70, 45, 90), 2, 10));
        QVERIFY(r45.has_value());
        QCOMPARE(r45->kind, RecognizedShape::Kind::Ellipse);
        QVERIFY(std::abs(r45->angleDegrees - 45) < 6);
    }
    void squareIsASquare()
    {
        const std::vector<QPointF> drawn{{100, 100}, {300, 100}, {300, 300}, {100, 300}};
        const auto r = recognizeShape(noisy(polyline(drawn, 25), 3, 4));
        QVERIFY(r.has_value());
        QCOMPARE(r->kind, RecognizedShape::Kind::Square);
        QVERIFY(std::abs(r->bounds.width() - 200) < 8);
        QVERIFY(std::abs(r->bounds.height() - 200) < 8);
        QVERIFY(sameCorners(r->corners, drawn));
    }
    void tiltedRectangleKeepsItsCorners()
    {
        // 223,6 × 111,8 incliné de 26,57° (atan(1/2)) : le cadre rendu, non tourné, et son angle redonnent
        // les coins.
        const std::vector<QPointF> drawn{{200, 100}, {400, 200}, {350, 300}, {150, 200}};
        const auto r = recognizeShape(noisy(polyline(drawn, 25), 2, 5));
        QVERIFY(r.has_value());
        QCOMPARE(r->kind, RecognizedShape::Kind::Rectangle);
        QCOMPARE(r->corners.size(), std::size_t{4});
        QVERIFY(std::abs(r->angleDegrees - 26.57) < 3);
        QVERIFY(std::abs(r->bounds.width() - 223.6) < 8);
        QVERIFY(std::abs(r->bounds.height() - 111.8) < 8);
        QVERIFY(sameCorners(r->corners, drawn));
        // Le même, commencé au milieu d'un grand côté : son premier côté est un petit côté (116,57°, un quart
        // de tour de plus) ; largeur et hauteur s'échangent pour rendre le même cadre.
        const auto rMid = recognizeShape(
            noisy(polyline({{300, 150}, {400, 200}, {350, 300}, {150, 200}, {200, 100}}, 25), 2, 12));
        QVERIFY(rMid.has_value());
        QCOMPARE(rMid->kind, RecognizedShape::Kind::Rectangle);
        QVERIFY(std::abs(rMid->angleDegrees - 26.57) < 3);
        QVERIFY(std::abs(rMid->bounds.width() - 223.6) < 8);
        QVERIFY(std::abs(rMid->bounds.height() - 111.8) < 8);
        QVERIFY(sameCorners(rMid->corners, drawn));
        // 200 × 100 incliné à 45° : boîte englobante carrée. Classer par la boîte et le rapport de ses côtés
        // en ferait un carré ; le rapport des côtés du tracé dit rectangle.
        const std::vector<QPointF> drawn45{{265, 194}, {406, 335}, {335, 406}, {194, 265}};
        const auto r45 = recognizeShape(noisy(polyline(drawn45, 25), 2, 11));
        QVERIFY(r45.has_value());
        QCOMPARE(r45->kind, RecognizedShape::Kind::Rectangle);
        QCOMPARE(r45->corners.size(), std::size_t{4});
        QVERIFY(sameCorners(r45->corners, drawn45));
    }
    void triangleIsATriangle()
    {
        const std::vector<QPointF> drawn{{300, 100}, {450, 350}, {150, 350}};
        const auto r = recognizeShape(noisy(polyline(drawn, 30), 2, 6));
        QVERIFY(r.has_value());
        QCOMPARE(r->kind, RecognizedShape::Kind::Triangle);
        QVERIFY(sameCorners(r->corners, drawn));
    }
    void roughQuadrilateralIsAPolygon()
    {
        // trapèze : angles de 76° et 104°, hors de 90° ± 10°
        const std::vector<QPointF> drawn{{100, 100}, {300, 100}, {350, 300}, {50, 300}};
        const auto r = recognizeShape(noisy(polyline(drawn, 25), 2, 8));
        QVERIFY(r.has_value());
        QCOMPARE(r->kind, RecognizedShape::Kind::Polygon);
        QCOMPARE(r->corners.size(), std::size_t{4});
        QVERIFY(sameCorners(r->corners, drawn));
    }
    void pentagonIsAPolygon()
    {
        const std::vector<QPointF> drawn{{300, 100}, {440, 200}, {390, 360}, {210, 360}, {160, 200}};
        const auto r = recognizeShape(noisy(polyline(drawn, 25), 2, 9));
        QVERIFY(r.has_value());
        QCOMPARE(r->kind, RecognizedShape::Kind::Polygon);
        QCOMPARE(r->corners.size(), std::size_t{5});
        QVERIFY(sameCorners(r->corners, drawn));
    }
    void openScribbleIsNothing()
    {
        QVERIFY(!recognizeShape(
                     noisy({{100, 100}, {180, 260}, {120, 90}, {300, 310}, {90, 200}, {260, 80}}, 1, 7))
                     .has_value());
    }
    void spiralIsNothing()
    {
        auto pts = ellipse({300, 300}, 100, 100, 0, 120, 2.2);
        for (std::size_t i = 0; i < pts.size(); ++i) // rayon qui croît : pas un cercle
            pts[i] = QPointF(300, 300) +
                     (pts[i] - QPointF(300, 300)) *
                         (0.4 + 0.6 * static_cast<double>(i) / static_cast<double>(pts.size()));
        QVERIFY(!recognizeShape(pts).has_value());
    }
    void shortStrokeIsNothing() { QVERIFY(!recognizeShape({{10, 10}, {14, 12}}).has_value()); }
};

QTEST_GUILESS_MAIN(TstFeaturesShapeFinder)
#include "tst_features_shapefinder.moc"
