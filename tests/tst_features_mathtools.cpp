#include <actcraft/features/mathtools.h>
#include <actcraft/features/strokeguide.h>

#include <QtTest>

using namespace actcraft;
using namespace actcraft::features;

namespace {
input::Sample at(double x, double y, input::Phase phase)
{
    input::Sample s;
    s.x = x;
    s.y = y;
    s.phase = phase;
    s.device = input::DeviceKind::Stylus;
    return s;
}
} // namespace

class TstFeaturesMathTools : public QObject
{
    Q_OBJECT
private slots:
    void rulerSnapsNearEdge()
    {
        Ruler r; // bord haut : de (200, 400) à (800, 400), voir rulerEdges()
        r.setFrame(QPointF(500, 440), 600, 80, 0); // centre, longueur, largeur, angle en degrés
        const auto snapped = r.capture(at(300, 405, input::Phase::Press), 16);
        QVERIFY(snapped.has_value());
        QCOMPARE(snapped->y, 400.0);
        QCOMPARE(snapped->x, 300.0);
    }
    void rulerIgnoresFarSamples()
    {
        Ruler r;
        r.setFrame(QPointF(500, 440), 600, 80, 0);
        QVERIFY(!r.capture(at(300, 300, input::Phase::Press), 16).has_value());
    }
    void strokeStaysOnTheEdgeOnceSnapped()
    {
        Ruler r;
        r.setFrame(QPointF(500, 440), 600, 80, 0);
        GuideSet guides;
        guides.add(&r);
        guides.setTolerance(16);
        QCOMPARE(guides.filter(at(300, 405, input::Phase::Press)).y, 400.0);
        QCOMPARE(guides.filter(at(500, 360, input::Phase::Move)).y, 400.0); // loin du bord : reste dessus
        QCOMPARE(guides.filter(at(900, 420, input::Phase::Move)).x, 800.0); // borné à la longueur
        // À 5 px du bord bas (y = 480), à 75 du bord haut : un guide re-saisi à chaque échantillon
        // y sauterait.
        QCOMPARE(guides.filter(at(500, 475, input::Phase::Move)).y, 400.0);
        QCOMPARE(guides.filter(at(600, 380, input::Phase::Release)).y, 400.0);
        QCOMPARE(guides.filter(at(300, 300, input::Phase::Press)).y, 300.0); // nouvel appui loin : libre
        QVERIFY(!guides.guiding());
        // Appui près du bord, puis appui loin sans relâché (relâché perdu) : le premier guide est lâché.
        QCOMPARE(guides.filter(at(300, 405, input::Phase::Press)).y, 400.0);
        QCOMPARE(guides.filter(at(300, 300, input::Phase::Press)).y, 300.0);
        QVERIFY(!guides.guiding());
    }
    void rotatedRulerProjects()
    {
        Ruler r;
        r.setFrame(QPointF(500, 500), 400, 80, 90); // verticale : bords à x = 460 et x = 540
        const auto s = r.capture(at(465, 520, input::Phase::Press), 16);
        QVERIFY(s.has_value());
        QCOMPARE(qRound(s->x), 460);
    }
    void angleDisplayedInWholeDegrees()
    {
        Ruler r;
        r.setFrame(QPointF(0, 0), 400, 80, 359.6);
        QCOMPARE(r.displayAngle(), 0);
        // Sens trigonométrique, comme la référence (y vers le bas : un angle positif tourne dans le sens des
        // aiguilles d'une montre à l'écran) : -30 s'affiche 30, +30 s'affiche 330.
        r.setFrame(QPointF(0, 0), 400, 80, -30);
        QCOMPARE(r.displayAngle(), 30);
        r.setFrame(QPointF(0, 0), 400, 80, 30);
        QCOMPARE(r.displayAngle(), 330);
    }
    void centimetreScale() { QCOMPARE(kPixelsPerCentimetre, 96.0 / 2.54); }
    void setSquareHasThreeEdges()
    {
        SetSquare q;
        q.setFrame(QPointF(400, 400), 300, 0); // sommet de l'angle droit, longueur des côtés, angle
        QCOMPARE(q.edges().size(), std::size_t{3});
        // À 90° (sens des aiguilles à l'écran) : le côté +x pointe vers le bas. Une rotation inversée le
        // mettrait en (400, 100) ; la règle tournée de 90°, symétrique, ne la voit pas.
        q.setFrame(QPointF(400, 400), 300, 90);
        QCOMPARE(q.edges()[0].p2().toPoint(), QPoint(400, 700));
        QCOMPARE(q.displayAngle(), 270);
    }
    void setSquareHypotenuseAt45()
    {
        SetSquare q;
        q.setFrame(QPointF(400, 400), 300, 0);
        const auto s =
            q.capture(at(552, 552, input::Phase::Press), 16); // près de l'hypoténuse (700,400)-(400,700)
        QVERIFY(s.has_value());
        QCOMPARE(qRound(s->x + s->y), 1100);
        // Suivi : le trait reste sur l'hypoténuse saisie, même passé plus près du côté +x (y = 400, à 20 px).
        const auto along = q.follow(at(450, 420, input::Phase::Move));
        QCOMPARE(qRound(along.x + along.y), 1100);
        QCOMPARE(qRound(along.x), 565);
        const auto end = q.follow(at(900, 100, input::Phase::Move)); // borné au coin (700, 400)
        QCOMPARE(end.x, 700.0);
        QCOMPARE(end.y, 400.0);
    }
    void guideSetPicksTheNearestGuide()
    {
        Ruler r;
        r.setFrame(QPointF(500, 440), 600, 80, 0); // bord haut à y = 400
        SetSquare q;
        q.setFrame(QPointF(300, 410), 300, 0); // côté horizontal à y = 410
        GuideSet guides;
        guides.add(&q); // l'équerre ajoutée d'abord : « le premier ajouté gagne » échouerait
        guides.add(&r);
        guides.setTolerance(16);
        QCOMPARE(guides.filter(at(350, 402, input::Phase::Press)).y,
                 400.0); // 2 px de la règle, 8 de l'équerre
        QVERIFY(guides.guiding());
        QCOMPARE(guides.filter(at(360, 412, input::Phase::Move)).y, 400.0); // garde la règle
        guides.filter(at(370, 400, input::Phase::Release));
        QVERIFY(!guides.guiding());
        guides.remove(&q); // l'équerre ajoutée en dernier : « le dernier ajouté gagne » échouerait aussi
        guides.add(&q);
        QCOMPARE(guides.filter(at(350, 402, input::Phase::Press)).y, 400.0);
        guides.filter(at(360, 402, input::Phase::Release));
        QVERIFY(!guides.guiding());
        // Le guide actif retiré pendant un trait : plus de guide actif (aucun pointeur gardé), la suite du
        // trait est libre, et le guide retiré ne saisit plus rien.
        QCOMPARE(guides.filter(at(350, 402, input::Phase::Press)).y, 400.0);
        guides.remove(&r);
        QVERIFY(!guides.guiding());
        QCOMPARE(guides.filter(at(360, 395, input::Phase::Move)).y, 395.0);
        guides.filter(at(360, 395, input::Phase::Release));
        QCOMPARE(guides.filter(at(350, 398, input::Phase::Press)).y, 410.0); // l'équerre seule, à 12 px
    }
    void snapPreviewIsQuietWhileGuiding()
    {
        Ruler r;
        r.setFrame(QPointF(500, 440), 600, 80, 0);
        GuideSet guides;
        guides.add(&r);
        guides.setTolerance(16);
        QVERIFY(
            guides.snapPreview(at(300, 405, input::Phase::Move)).has_value()); // survol : cercle d'accroche
        guides.filter(at(300, 405, input::Phase::Press));
        QVERIFY(!guides.snapPreview(at(300, 475, input::Phase::Move)).has_value()); // pendant le trait : rien
        QCOMPARE(guides.filter(at(400, 470, input::Phase::Move)).y, 400.0);         // toujours le bord haut
        guides.filter(at(400, 470, input::Phase::Release));
        // Survol après le relâché, près du bord bas : l'aperçu le montre sans rien changer au guide, qui
        // suit encore le bord saisi par le dernier trait.
        const auto hover = guides.snapPreview(at(300, 475, input::Phase::Move));
        QVERIFY(hover.has_value());
        QCOMPARE(hover->y, 480.0);
        QCOMPARE(r.follow(at(400, 470, input::Phase::Move)).y, 400.0);
    }
    void sampleKeepsItsOtherFields()
    {
        Ruler r;
        r.setFrame(QPointF(500, 440), 600, 80, 0);
        GuideSet guides;
        guides.add(&r);
        auto s = at(300, 405, input::Phase::Press);
        s.pressure = 0.25f;
        s.timestampNs = 42;
        s.seq = 7;
        const auto out = guides.filter(s);
        QCOMPARE(out.y, 400.0); // projeté : sans lui, un filtre qui ne fait rien passerait le reste
        QCOMPARE(out.pressure, 0.25f);
        QCOMPARE(out.timestampNs, std::int64_t{42});
        QCOMPARE(out.seq, std::uint64_t{7});
        QCOMPARE(out.phase, input::Phase::Press);
        QCOMPARE(out.device, input::DeviceKind::Stylus);
    }
};

QTEST_GUILESS_MAIN(TstFeaturesMathTools)
#include "tst_features_mathtools.moc"
