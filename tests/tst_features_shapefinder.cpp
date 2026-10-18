#include <actcraft/features/shapefinder.h>

#include <QRandomGenerator>
#include <QtTest>

#include <cmath>
#include <numbers>

using namespace actcraft::features;

namespace {
std::vector<QPointF> noisy(std::vector<QPointF> pts, double amplitude, quint32 seed)
{
    QRandomGenerator g(seed);
    for (auto& p : pts)
        p += QPointF((g.generateDouble() * 2 - 1) * amplitude, (g.generateDouble() * 2 - 1) * amplitude);
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
        const auto r =
            recognizeShape(noisy(polyline({{100, 100}, {300, 100}, {300, 300}, {100, 300}}, 25), 3, 4));
        QVERIFY(r.has_value());
        QCOMPARE(r->kind, RecognizedShape::Kind::Square);
    }
    void tiltedRectangleKeepsItsCorners()
    {
        const auto r =
            recognizeShape(noisy(polyline({{200, 100}, {400, 200}, {350, 300}, {150, 200}}, 25), 2, 5));
        QVERIFY(r.has_value());
        QCOMPARE(r->kind, RecognizedShape::Kind::Rectangle);
        QCOMPARE(r->corners.size(), std::size_t{4});
        // 200 × 100 incliné à 45° : boîte englobante carrée. Classer par la boîte et le rapport de ses côtés
        // en ferait un carré ; le rapport des côtés du tracé dit rectangle.
        const auto r45 =
            recognizeShape(noisy(polyline({{265, 194}, {406, 335}, {335, 406}, {194, 265}}, 25), 2, 11));
        QVERIFY(r45.has_value());
        QCOMPARE(r45->kind, RecognizedShape::Kind::Rectangle);
        QCOMPARE(r45->corners.size(), std::size_t{4});
    }
    void triangleIsATriangle()
    {
        const auto r = recognizeShape(noisy(polyline({{300, 100}, {450, 350}, {150, 350}}, 30), 2, 6));
        QVERIFY(r.has_value());
        QCOMPARE(r->kind, RecognizedShape::Kind::Triangle);
    }
    void roughQuadrilateralIsAPolygon()
    {
        // trapèze : angles de 76° et 104°, hors de 90° ± 10°
        const auto r =
            recognizeShape(noisy(polyline({{100, 100}, {300, 100}, {350, 300}, {50, 300}}, 25), 2, 8));
        QVERIFY(r.has_value());
        QCOMPARE(r->kind, RecognizedShape::Kind::Polygon);
        QCOMPARE(r->corners.size(), std::size_t{4});
    }
    void pentagonIsAPolygon()
    {
        const auto r = recognizeShape(
            noisy(polyline({{300, 100}, {440, 200}, {390, 360}, {210, 360}, {160, 200}}, 25), 2, 9));
        QVERIFY(r.has_value());
        QCOMPARE(r->kind, RecognizedShape::Kind::Polygon);
        QCOMPARE(r->corners.size(), std::size_t{5});
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
