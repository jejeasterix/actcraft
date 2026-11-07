#include <actcraft/core/edits.h>
#include <actcraft/core/interactivity.h>
#include <actcraft/core/model.h>
#include <actcraft/core/restrictors.h>

#include <QtTest>

#include <cmath>
#include <utility>

using namespace actcraft::core;

namespace {

// Emprise en (0, 0) et position par la transformation : un calcul fait dans les coordonnées de l'objet (au
// lieu de celles de la page) tombe à côté. Sans contour : l'emprise sur la page (boundsOnPage, épaisseur
// comprise) est exactement w × h.
PageObject shapeAt(ObjectId id, double x, double y, double w, double h)
{
    Shape shape;
    shape.bounds = {0, 0, w, h};
    shape.outlineWidth = 0;
    PageObject object(std::move(shape));
    object.common.id = id;
    object.common.transform = translation(x, y);
    return object;
}

// Montage de L8-T14 : forme 1 de 40 × 40 en (100, 100) ; trait 2 de (0, 300) à (400, 300) ; conteneur 3 en
// (300, 400, 100, 80) ; forme bloquante 4 en (200, 100, 40, 40) ; trait coudé 5 (500, 0) → (600, 0) → (600,
// 100).
Page montage()
{
    Page page;
    page.objects.push_back(shapeAt(1, 100, 100, 40, 40));

    Line track;
    track.points = {{0, 0}, {400, 0}};
    PageObject line(std::move(track));
    line.common.id = 2;
    line.common.transform = translation(0, 300);
    page.objects.push_back(std::move(line));

    PageObject box = shapeAt(3, 300, 400, 100, 80);
    box.common.container.accepts = Accepts::Anything;
    page.objects.push_back(std::move(box));

    PageObject wall = shapeAt(4, 200, 100, 40, 40);
    wall.common.restrictions.blocksOthers = true;
    page.objects.push_back(std::move(wall));

    Stroke bent;
    bent.points = {{0, 0, 0.5f}, {100, 0, 0.5f}, {100, 100, 0.5f}};
    PageObject stroke(std::move(bent));
    stroke.common.id = 5;
    stroke.common.transform = translation(500, 0);
    page.objects.push_back(std::move(stroke));
    return page;
}

Restrictions& restrictionsOfShape(Page& page)
{
    return page.objects[0].common.restrictions;
}

} // namespace

class TstCoreRestrictors : public QObject
{
    Q_OBJECT
private slots:
    void verticalKeepsX()
    {
        Page page = montage();
        setTravel(restrictionsOfShape(page), Travel::Vertical);
        const PointF travel = allowedTravel(page, page.objects[0], {30, 50});
        QCOMPARE(qRound(travel.x), 0);
        QCOMPARE(qRound(travel.y), 50);
    }

    void horizontalKeepsY()
    {
        Page page = montage();
        setTravel(restrictionsOfShape(page), Travel::Horizontal);
        const PointF travel = allowedTravel(page, page.objects[0], {30, 50});
        QCOMPARE(qRound(travel.x), 30);
        QCOMPARE(qRound(travel.y), 0);
    }

    void fixedTravelZero()
    {
        Page page = montage();
        setTravel(restrictionsOfShape(page), Travel::Fixed);
        PointF travel = allowedTravel(page, page.objects[0], {30, 50});
        QCOMPARE(qRound(travel.x), 0);
        QCOMPARE(qRound(travel.y), 0);

        // Objet bloqué lu d'un fichier (lot 2) : seul le booléen est faux, le mode reste libre. Le booléen
        // est la source du « bloqué » (D8-7) : l'objet ne bouge pas.
        Page read = montage();
        restrictionsOfShape(read).movable = false;
        QCOMPARE(restrictionsOfShape(read).travel, Travel::Free);
        travel = allowedTravel(read, read.objects[0], {30, 50});
        QCOMPARE(qRound(travel.x), 0);
        QCOMPARE(qRound(travel.y), 0);
        // Ni saut sur un chemin.
        setTravel(restrictionsOfShape(read), Travel::AlongTrack);
        restrictionsOfShape(read).movable = false;
        restrictionsOfShape(read).track = 2;
        const PointF still = projectOnTrack(read, read.objects[0], {200, 250});
        QCOMPARE(qRound(still.x), 200);
        QCOMPARE(qRound(still.y), 250);
    }

    void alongTrackProjects()
    {
        Page page = montage();
        setTravel(restrictionsOfShape(page), Travel::AlongTrack);
        restrictionsOfShape(page).track = 2;
        const PointF onLine = projectOnTrack(page, page.objects[0], {200, 250});
        QCOMPARE(qRound(onLine.x), 200);
        QCOMPARE(qRound(onLine.y), 300);

        // Polyligne : projection sur chaque segment, pas sur le sommet le plus proche.
        restrictionsOfShape(page).track = 5;
        const PointF onStroke = projectOnTrack(page, page.objects[0], {650, 50});
        QCOMPARE(qRound(onStroke.x), 600);
        QCOMPARE(qRound(onStroke.y), 50);
    }

    void jumpsOntoTrackOnPress()
    {
        Page page = montage();
        setTravel(restrictionsOfShape(page), Travel::AlongTrack);
        restrictionsOfShape(page).track = 2;
        const PointF jumped = projectOnTrack(page, page.objects[0], {120, 120});
        QCOMPARE(qRound(jumped.x), 120);
        QCOMPARE(qRound(jumped.y), 300);
        // À l'appui (aucun déplacement voulu), le déplacement permis porte le centre de la forme sur le
        // trait.
        const PointF travel = allowedTravel(page, page.objects[0], {0, 0});
        QCOMPARE(qRound(travel.x), 0);
        QCOMPARE(qRound(travel.y), 180);
    }

    void landsOnTheMark()
    {
        Page page = montage();
        PointF centre = landingPoint(page, page.objects[0], {330, 430});
        QCOMPARE(qRound(centre.x), 330); // sans aimant, l'objet reste où il est lâché
        QCOMPARE(qRound(centre.y), 430);

        restrictionsOfShape(page).magnet = true;
        restrictionsOfShape(page).magnetMark = Mark::TopLeft;
        centre = landingPoint(page, page.objects[0], {330, 430});
        // Coin haut gauche de la forme sur celui du conteneur 3 (300, 400) : centre en (320, 420).
        QCOMPARE(qRound(centre.x), 320);
        QCOMPARE(qRound(centre.y), 420);

        // Point précis (coordonnées de page) : le centre s'y pose, conteneur ou non sous le point.
        restrictionsOfShape(page).magnetMark = Mark::Point;
        restrictionsOfShape(page).magnetPoint = {500, 600};
        centre = landingPoint(page, page.objects[0], {330, 430});
        QCOMPARE(qRound(centre.x), 500);
        QCOMPARE(qRound(centre.y), 600);
    }

    void fixedSizeHasNoHandles()
    {
        Page page = montage();
        QVERIFY(sizeHandlesShown(page.objects[0]));
        restrictionsOfShape(page).resizable = false;
        QVERIFY(!sizeHandlesShown(page.objects[0]));
    }

    void clockwiseOnly()
    {
        Page page = montage();
        setTurning(restrictionsOfShape(page), Turning::Clockwise);
        QCOMPARE(qRound(allowedTurn(page.objects[0], 10, 0)), 10);
        QCOMPARE(qRound(allowedTurn(page.objects[0], 10, 30)), 30);

        // Rotation bloquée lue d'un fichier (lot 2) : booléen faux, mode libre ; aucun sens n'est permis.
        Page read = montage();
        restrictionsOfShape(read).rotatable = false;
        QCOMPARE(qRound(allowedTurn(read.objects[0], 10, 30)), 10);
        QCOMPARE(qRound(allowedTurn(read.objects[0], 10, 0)), 10);
    }

    void turnStepRounds()
    {
        Page page = montage();
        restrictionsOfShape(page).turnStep = 45;
        QCOMPARE(qRound(allowedTurn(page.objects[0], 0, 50)), 45);
        QCOMPARE(qRound(allowedTurn(page.objects[0], 0, 70)), 90);
    }

    void pivotOnOtherObject()
    {
        Page page = montage();
        PointF pivot = pivotOf(page, page.objects[0]);
        QCOMPARE(qRound(pivot.x), 120); // défaut : centre de la forme, en coordonnées de page
        QCOMPARE(qRound(pivot.y), 120);

        restrictionsOfShape(page).pivot = Mark::OtherObject;
        restrictionsOfShape(page).pivotObject = 3;
        pivot = pivotOf(page, page.objects[0]);
        QCOMPARE(qRound(pivot.x), 350);
        QCOMPARE(qRound(pivot.y), 440);
    }

    void blockerStopsAtItsEdge()
    {
        const Page page = montage();
        // La forme passerait par-dessus le bloquant 4 et se poserait derrière lui : elle s'arrête bord contre
        // bord.
        PointF travel = stopAtBlockers(page, {1}, {150, 0});
        QCOMPARE(qRound(travel.x), 60);
        QCOMPARE(qRound(travel.y), 0);
        // Un déplacement qui n'atteint pas le bloquant reste entier.
        travel = stopAtBlockers(page, {1}, {40, 0});
        QCOMPARE(qRound(travel.x), 40);
        QCOMPARE(qRound(travel.y), 0);

        // Le bord est celui de l'emprise sur la page (boundsOnPage), contour compris : avec un contour de 1
        // sur chaque forme, les encres se touchent un pixel plus tôt.
        Page outlined = montage();
        std::get<Shape>(outlined.objects[0]).outlineWidth = 1;
        std::get<Shape>(outlined.objects[3]).outlineWidth = 1;
        travel = stopAtBlockers(outlined, {1}, {150, 0});
        QCOMPARE(qRound(travel.x), 59);
        QCOMPARE(qRound(travel.y), 0);

        // Glisser le long du bord bas du bloquant (aucun déplacement vertical) : rien ne l'arrête.
        Page beside = montage();
        beside.objects[0].common.transform = translation(100, 140);
        travel = stopAtBlockers(beside, {1}, {150, 0});
        QCOMPARE(qRound(travel.x), 150);
        QCOMPARE(qRound(travel.y), 0);
    }

    void secondDragStaysAtTheEdge()
    {
        // Après un arrêt bord contre bord, la position t·wanted peut recouvrir le bloquant d'un ulp : les
        // glissers suivants dans le même sens ne le traversent pas, quelle que soit la position de départ.
        for (int i = 0; i < 200; ++i) {
            Page page = montage();
            PageObject& shape = page.objects[0];
            shape.common.transform = translation(100 + i * 0.0137, 100.3 + i * 0.011);
            const PointF wanted{150.3 + i * 0.071, 0};
            for (int drag = 0; drag < 3; ++drag) {
                const PointF travel = stopAtBlockers(page, {1}, wanted);
                shape.common.transform.dx += travel.x;
                shape.common.transform.dy += travel.y;
            }
            const RectF box = boundsOnPage(page, shape);
            QVERIFY2(box.x + box.width <= 200 + 1e-5, qPrintable(QStringLiteral("départ %1").arg(i)));
        }
    }

    void eachMovingObjectStops()
    {
        // Deux objets soulevés de part et d'autre du bloquant 4 : la boîte de chacun compte, pas leur union
        // (qui recouvre déjà le bloquant).
        Page page = montage();
        page.objects.push_back(shapeAt(6, 300, 100, 40, 40));
        const PointF travel = stopAtBlockers(page, {1, 6}, {150, 0});
        QCOMPARE(qRound(travel.x), 60);
        QCOMPARE(qRound(travel.y), 0);
    }

    void blockerBoxesMatchThePage()
    {
        // Variante sur boîtes précalculées (L8-T16, revue) : le même arrêt que la variante sur page, boîte
        // par boîte ; sans bloquant, rien n'est réduit.
        Page page = montage();
        page.objects.push_back(shapeAt(6, 300, 100, 40, 40));
        const std::vector<RectF> moving{boundsOnPage(page, page.objects[0]),
                                        boundsOnPage(page, page.objects.back())};
        std::vector<RectF> blockers;
        for (const PageObject& o : page.objects) {
            if (o.common.restrictions.blocksOthers && o.common.id != 1 && o.common.id != 6 &&
                hasExtent(page, o)) {
                blockers.push_back(boundsOnPage(page, o));
            }
        }
        QVERIFY(!blockers.empty());
        for (const PointF wanted : {PointF{150, 0}, PointF{40, 0}, PointF{-90, 30}, PointF{150, 12}}) {
            const PointF byPage = stopAtBlockers(page, {1, 6}, wanted);
            const PointF byBoxes = stopBoxesAtBlockers(moving, blockers, wanted);
            QCOMPARE(byBoxes.x, byPage.x);
            QCOMPARE(byBoxes.y, byPage.y);
        }
        const PointF free = stopBoxesAtBlockers(moving, {}, {150, 0});
        QCOMPARE(free.x, 150.0);
        QCOMPARE(qRound(stopBoxesAtBlockers(moving, blockers, {150, 0}).x), 60);
    }

    void fullTurnInOneDrag()
    {
        // Contrat : `fromDegrees` est le dernier angle permis (rendu au mouvement précédent). L'angle voulu
        // est celui du pointeur, ramené dans ]-180, 180] : il saute de 360° au passage de ±180°. Le résultat
        // reste continu avec le précédent.
        Page page = montage();
        setTurning(restrictionsOfShape(page), Turning::Clockwise);
        const PageObject& shape = page.objects[0];
        double allowed = 0;
        for (int k = 1; k <= 36; ++k) {
            allowed = allowedTurn(shape, allowed, std::remainder(k * 10.0, 360.0));
            QCOMPARE(qRound(allowed), k * 10);
        }
        // À rebours (jusqu'à 170°), rien ne tourne.
        allowed = 0;
        for (int k = 1; k <= 17; ++k) {
            allowed = allowedTurn(shape, allowed, -k * 10.0);
            QCOMPARE(qRound(allowed), 0);
        }
        // Avec un pas de 45° : chaque angle permis est le multiple le plus proche, tour complet compris.
        restrictionsOfShape(page).turnStep = 45;
        allowed = 0;
        for (int k = 1; k <= 36; ++k) {
            allowed = allowedTurn(shape, allowed, std::remainder(k * 10.0, 360.0));
            QCOMPARE(qRound(allowed), qRound(45 * std::round(k * 10 / 45.0)));
        }
    }

    void counterClockwiseOnly()
    {
        Page page = montage();
        setTurning(restrictionsOfShape(page), Turning::CounterClockwise);
        const PageObject& shape = page.objects[0];
        QCOMPARE(qRound(allowedTurn(shape, 10, 30)), 10);
        QCOMPARE(qRound(allowedTurn(shape, 10, 0)), 0);
        double allowed = 0;
        for (int k = 1; k <= 36; ++k) {
            allowed = allowedTurn(shape, allowed, std::remainder(-k * 10.0, 360.0));
            QCOMPARE(qRound(allowed), -k * 10);
        }
    }

    void pivotOnPoint()
    {
        Page page = montage();
        restrictionsOfShape(page).pivot = Mark::Point;
        restrictionsOfShape(page).pivotPoint = {10, 30}; // coordonnées de l'objet
        PointF pivot = pivotOf(page, page.objects[0]);
        QCOMPARE(qRound(pivot.x), 110);
        QCOMPARE(qRound(pivot.y), 130);
        restrictionsOfShape(page).pivot = Mark::BottomRight;
        pivot = pivotOf(page, page.objects[0]);
        QCOMPARE(qRound(pivot.x), 140);
        QCOMPARE(qRound(pivot.y), 140);
    }
};

QTEST_MAIN(TstCoreRestrictors)
#include "tst_core_restrictors.moc"
