#include <actcraft/features/mathtools.h>
#include <actcraft/features/strokeguide.h>

#include <QtMath>
#include <QtTest>

#include <cmath>

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
    void protractorArcFollowsTheRim()
    {
        Protractor p;
        p.setFrame(QPointF(500, 500), 200, 0, Protractor::Kind::Full);          // centre, rayon, angle, forme
        QVERIFY(!p.capture(at(600, 500, input::Phase::Press), 16).has_value()); // loin du pourtour : rien
        const auto s = p.capture(at(705, 500, input::Phase::Press), 16);
        QVERIFY(s.has_value());
        QCOMPARE(qRound(std::hypot(s->x - 500, s->y - 500)), 200);
        const auto f = p.follow(at(500, 300, input::Phase::Move));
        QCOMPARE(qRound(std::hypot(f.x - 500, f.y - 500)), 200);
        QCOMPARE(qRound(p.sweptDegrees()), 90);
        // Vers le haut de l'écran, le balayage signé est négatif (repère de page) : repris tel quel par
        // arcPath, l'arc plein couvre le quart tracé, au-dessus du centre, et non son symétrique.
        QCOMPARE(qRound(p.signedSweptDegrees()), -90);
        const auto sector =
            arcPath(QPointF(500, 500), 200, p.startDegrees(), p.signedSweptDegrees(), ArcStyle::Filled);
        QCOMPARE(sector[sector.size() - 2].toPoint(), QPoint(500, 300)); // fin de l'arc : le point suivi
        QVERIFY(sector[sector.size() / 2].y() < 500);                    // milieu de l'arc : au-dessus
        // Passé le rayon gauche (où atan2 saute de 180 à -180), à 50 px du pourtour, dehors puis dedans, le
        // trait suit le cercle saisi à l'appui (une re-saisie par échantillon le lâcherait), et l'angle
        // cumulé continue (une différence d'atan2 brute entre l'angle actuel et celui du départ retomberait à
        // 90).
        const auto g = p.follow(at(250, 500, input::Phase::Move));
        QCOMPARE(qRound(g.x), 300);
        QCOMPARE(qRound(g.y), 500);
        QCOMPARE(qRound(p.sweptDegrees()), 180);
        const auto b = p.follow(at(500, 650, input::Phase::Move));
        QCOMPARE(qRound(std::hypot(b.x - 500, b.y - 500)), 200);
        QCOMPARE(qRound(p.sweptDegrees()), 270);
        // À travers un GuideSet : le pourtour est trouvé par project() (guide le plus proche), saisi, suivi.
        GuideSet guides;
        guides.add(&p);
        guides.setTolerance(16);
        QCOMPARE(qRound(guides.filter(at(500, 295, input::Phase::Press)).y), 300); // 5 px hors du pourtour
        QVERIFY(guides.guiding());
        QCOMPARE(qRound(guides.filter(at(650, 500, input::Phase::Move)).x), 700); // 50 px dedans : ramené
    }
    void halfProtractorStopsAt180()
    {
        Protractor p;
        p.setFrame(QPointF(500, 500), 200, 0, Protractor::Kind::Half); // demi-disque au-dessus du diamètre
        QVERIFY(!p.capture(at(500, 705, input::Phase::Press), 16).has_value()); // sous le diamètre : rien
        // Passé sous le diamètre, le trait reste au bout le plus proche (le gauche), l'angle reste 180 :
        // borné par un simple clamp dans [-180, 0], l'angle local 153 deviendrait 0, et le trait sauterait au
        // bout droit.
        QVERIFY(p.capture(at(705, 500, input::Phase::Press), 16).has_value()); // bout droit
        QCOMPARE(qRound(p.follow(at(500, 250, input::Phase::Move)).y), 300);
        QCOMPARE(qRound(p.sweptDegrees()), 90);
        const auto end = p.follow(at(300, 600, input::Phase::Move));
        QCOMPARE(qRound(end.x), 300);
        QCOMPARE(qRound(end.y), 500);
        QCOMPARE(qRound(p.sweptDegrees()), 180);
        // Sous le diamètre côté droit (angle local 22) : le bout droit, le plus proche, et l'angle revient à
        // 0.
        const auto right = p.follow(at(650, 560, input::Phase::Move));
        QCOMPARE(qRound(right.x), 700);
        QCOMPARE(qRound(right.y), 500);
        QCOMPARE(qRound(p.sweptDegrees()), 0);
        // Un demi-tour d'un seul pas, puis un retour de 10° : 170, jamais plus de 180 (un pas de -180° ramené
        // à +180° donnerait 190).
        QVERIFY(p.capture(at(705, 500, input::Phase::Press), 16).has_value());
        p.follow(at(250, 500, input::Phase::Move));
        const double back = qDegreesToRadians(-170.0);
        p.follow(at(500 + 200 * std::cos(back), 500 + 200 * std::sin(back), input::Phase::Move));
        QCOMPARE(qRound(p.sweptDegrees()), 170);
        // Tourné de -90 (sens trigonométrique à l'écran) : le demi-disque est à gauche, l'angle s'affiche 90.
        p.setFrame(QPointF(500, 500), 200, -90, Protractor::Kind::Half);
        QVERIFY(!p.capture(at(705, 500, input::Phase::Press), 16).has_value());
        QVERIFY(p.capture(at(295, 500, input::Phase::Press), 16).has_value());
        QCOMPARE(p.displayAngle(), 90);
        // Suivi dans le repère tourné : le bout « droit » du diamètre est en haut, l'autre en bas.
        const auto top = p.follow(at(500, 250, input::Phase::Move));
        QCOMPARE(qRound(top.x), 500);
        QCOMPARE(qRound(top.y), 300);
        const auto bottom = p.follow(at(600, 650, input::Phase::Move)); // à droite, plus près du bout bas
        QCOMPARE(qRound(bottom.x), 500);
        QCOMPARE(qRound(bottom.y), 700);
    }
    void closedArcAddsTwoRadii()
    {
        const auto path = arcPath(QPointF(0, 0), 100, 0, 90, ArcStyle::Closed);
        QCOMPARE(path.front(), QPointF(0, 0));
        QCOMPARE(path.back(), QPointF(0, 0));
        // Le premier rayon va du centre au point saisi sur le rapporteur : startDegrees() est un angle de
        // page, comme ceux d'arcPath (un angle compté dans l'autre sens partirait du point symétrique, en
        // bas).
        Protractor p;
        p.setFrame(QPointF(0, 0), 100, 0, Protractor::Kind::Full);
        const auto s = p.capture(at(50, -87, input::Phase::Press), 16); // vers -60° : en haut à droite
        QVERIFY(s.has_value());
        const auto fromRim = arcPath(QPointF(0, 0), 100, p.startDegrees(), 90, ArcStyle::Closed);
        QCOMPARE(fromRim[1].toPoint(), QPoint(qRound(s->x), qRound(s->y)));
    }
    void compassDrawsAnArcAtItsRadius()
    {
        Compass c;
        c.setPivot(QPointF(400, 400));
        c.setRadius(150);
        c.setAngle(0); // mine à (550, 400)
        // Sur le cercle mais loin de la mine : rien (c'est la mine qui saisit le stylo, pas le cercle).
        QVERIFY(!c.capture(at(400, 550, input::Phase::Press), 16).has_value());
        QVERIFY(c.capture(at(553, 402, input::Phase::Press), 16).has_value());
        const auto f = c.follow(at(400, 200, input::Phase::Move));
        QCOMPARE(qRound(std::hypot(f.x - 400, f.y - 400)), 150);
        // La mine a suivi le stylo vers le haut de l'écran : 90 en sens trigonométrique (270 sinon).
        QCOMPARE(c.displayAngle(), 90);
        QCOMPARE(c.pencil().toPoint(), QPoint(400, 250));
        // Mine tournée : c'est sa position courante qui saisit, plus celle de l'angle 0, y compris à travers
        // un GuideSet (project(), guide le plus proche).
        QVERIFY(!c.capture(at(553, 402, input::Phase::Press), 16).has_value());
        GuideSet guides;
        guides.add(&c);
        guides.setTolerance(16);
        QCOMPARE(qRound(guides.filter(at(404, 252, input::Phase::Press)).y), 250);
        QVERIFY(guides.guiding());
        QCOMPARE(qRound(guides.filter(at(600, 400, input::Phase::Move)).x), 550); // ramené sur le cercle
    }
    void fullTurnClosesTheCircle()
    {
        Compass c;
        c.setPivot(QPointF(0, 0));
        c.setRadius(100);
        c.capture(at(100, 0, input::Phase::Press), 16);
        for (int d = 10; d <= 400; d += 10)
            c.follow(at(100 * std::cos(qDegreesToRadians(double(d))),
                        100 * std::sin(qDegreesToRadians(double(d))), input::Phase::Move));
        QVERIFY(c.sweptDegrees() >= 360);
        QVERIFY(c.closedCircle());
    }
    void radiusInCentimetres()
    {
        Compass c;
        c.setRadius(kPixelsPerCentimetre * 3);
        QCOMPARE(c.radiusText(), QStringLiteral("3,0 cm"));
        c.setRadius(5); // borné : 20 au moins, 2000 au plus
        QCOMPARE(c.radius(), Compass::kMinRadius);
        c.setRadius(5000);
        QCOMPARE(c.radius(), Compass::kMaxRadius);
    }
    void filledArcIsClosedOnTheCentre()
    {
        const auto path = arcPath(QPointF(10, 20), 50, 0, 90, ArcStyle::Filled);
        QCOMPARE(path.front(), QPointF(10, 20));
        QCOMPARE(path.back(), QPointF(10, 20));
        QCOMPARE(path.size(), std::size_t{48}); // centre + 46 points de l'arc (pas de 2°) + centre
        QCOMPARE(arcPath(QPointF(10, 20), 50, 0, 90, ArcStyle::Arc).size(), std::size_t{46}); // sans centre
        // Balayage négatif (vers le haut à l'écran) : autant de pas, compté sur sa valeur absolue.
        const auto up = arcPath(QPointF(10, 20), 50, 0, -90, ArcStyle::Filled);
        QCOMPARE(up.size(), std::size_t{48});
        QCOMPARE(up[up.size() - 2].toPoint(), QPoint(10, -30));
        // 5° : 3 pas, arrondis au-dessus (une troncature en ferait 2), jusqu'à 5° exactement.
        const auto fiveDegrees = arcPath(QPointF(0, 0), 100, 0, 5, ArcStyle::Arc);
        QCOMPARE(fiveDegrees.size(), std::size_t{4});
        QCOMPARE(fiveDegrees.back().toPoint(), QPoint(100, 9));
        // Balayage nul : un pas quand même, deux points au départ, sans division par zéro.
        const auto zero = arcPath(QPointF(0, 0), 100, 30, 0, ArcStyle::Arc);
        QCOMPARE(zero.size(), std::size_t{2});
        QVERIFY(std::isfinite(zero.back().x()) && std::isfinite(zero.back().y()));
        QCOMPARE(zero.back().toPoint(), QPoint(87, 50));
    }
    void compassClosesOnItsFirstPoint()
    {
        Compass c;
        c.setPivot(QPointF(0, 0));
        c.setRadius(100);
        const auto first = c.capture(at(100, 0, input::Phase::Press), 16);
        QVERIFY(first.has_value());
        input::Sample last;
        for (int d = 10; d <= 370; d += 10)
            last = c.follow(at(100 * std::cos(qDegreesToRadians(double(d))),
                               100 * std::sin(qDegreesToRadians(double(d))), input::Phase::Move));
        QVERIFY(c.closedCircle());
        QCOMPARE(last.x, first->x); // fermé exactement sur le premier point
        QCOMPARE(last.y, first->y);
        // Dans l'autre sens (vers le haut à l'écran) : le balayage, négatif, se lit positif et ferme aussi le
        // cercle (un « ≥ 360 » sur l'angle signé ne le fermerait jamais).
        Compass up;
        up.setPivot(QPointF(0, 0));
        up.setRadius(100);
        QVERIFY(up.capture(at(100, 0, input::Phase::Press), 16).has_value());
        for (int d = -10; d >= -370; d -= 10)
            last = up.follow(at(100 * std::cos(qDegreesToRadians(double(d))),
                                100 * std::sin(qDegreesToRadians(double(d))), input::Phase::Move));
        QCOMPARE(qRound(up.sweptDegrees()), 370);
        QVERIFY(up.closedCircle());
        QCOMPARE(last.x, first->x);
        QCOMPARE(last.y, first->y);
    }
    void originRememberedPerPage()
    {
        OriginMemory m(QSizeF(1000, 800));
        m.set(0, QPointF(100, 100));
        m.set(1, QPointF(300, 200));
        QCOMPARE(m.at(0), QPointF(100, 100));
        QCOMPARE(m.at(1), QPointF(300, 200));
        QVERIFY(m.has(0));
        QVERIFY(!m.has(2));
        QCOMPARE(m.at(2), QPointF(500, 400)); // défaut : centre de la page
        m.forget(1);
        QVERIFY(!m.has(1));
        QCOMPARE(m.at(1), QPointF(500, 400)); // oubliée : de nouveau le centre
        m.setPageSize(QSizeF(600, 400));
        QCOMPARE(m.at(2), QPointF(300, 200)); // le défaut suit la taille de la page
        QCOMPARE(m.at(0), QPointF(100, 100)); // une origine posée ne bouge pas
    }
};

QTEST_GUILESS_MAIN(TstFeaturesMathTools)
#include "tst_features_mathtools.moc"
