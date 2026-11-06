#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/interactivity.h>
#include <actcraft/core/objectactions.h>
#include <actcraft/core/objectedits.h>

#include <QtMath>
#include <QtTest>

#include <cmath>

using namespace actcraft::core;

// m10 : Transform n'a ni translation() ni rotationDegrees() ; deux aides locales, écrites d'après les champs
// de `struct Transform` (model.h : x' = m11·x + m21·y + dx ; y' = m12·x + m22·y + dy).
static Transform translated(double x, double y)
{
    return translation(x, y);
}
// Degrés, sens horaire à l'écran (convention de rotation(), model.h), d'après les termes de rotation de la
// matrice.
static double rotationOf(const Transform& t)
{
    return qRadiansToDegrees(std::atan2(t.m12, t.m11));
}

class TstCoreObjectActions : public QObject
{
    Q_OBJECT
    static ActionContext on(ObjectId source) { return ActionContext{0, source, 0, 1024, 768}; }
    static Document page(double width = 100, double height = 50)
    {
        Document d(1024, 768);
        Shape s;
        s.bounds = {0, 0, width, height};
        s.outlineWidth = 0; // boundsOnPage compte l'épaisseur (edits.h) : boîte = 100 × 50 exactement
        PageObject o{s};
        o.common.id = 1;
        o.common.transform = translated(200, 100);
        d.page(0).objects.push_back(o);
        return d;
    }
    static Document after(Document d, const char* id, const char* params)
    {
        auto c = objectActionCommand(d, id, params, on(1));
        [&] { QVERIFY2(c, id); }();
        Changes changes;
        c->redo(d, changes);
        return d;
    }
    static RectF box(const Document& d) { return boundsOnPage(d.page(0).objects[0]); }
    static double angleOf(const Document& d) { return rotationOf(d.page(0).objects[0].common.transform); }
    static const Transform& transformOf(const Document& d) { return d.page(0).objects[0].common.transform; }
private slots:
    void angleSetsAbsoluteRotation()
    {
        const Document d =
            after(after(page(), "object.angle", R"({"angle":30})"), "object.angle", R"({"angle":30})");
        QCOMPARE(qRound(angleOf(d)), 30);
        // Rotation autour du pivot de la cible (centre par défaut, D8-7) : le centre de la boîte ne bouge
        // pas.
        const RectF b = box(d);
        QCOMPARE(qRound(b.x + b.width / 2), 250);
        QCOMPARE(qRound(b.y + b.height / 2), 125);
    }
    void incrementNegativeIsCounterClockwise()
    {
        QCOMPARE(qRound(angleOf(after(page(), "object.angle-step", R"({"angle":-15})"))), -15);
    }
    void placeCenterUsesThePage()
    {
        const RectF b = box(after(page(), "object.place-center", "{}"));
        QCOMPARE(qRound(b.x + b.width / 2), 512);
        QCOMPARE(qRound(b.y + b.height / 2), 384);
    }
    void sizeKeepsProportions()
    {
        const RectF b = box(after(page(), "object.size-top-left", R"({"width":200})"));
        QCOMPARE(qRound(b.width), 200);
        QCOMPARE(qRound(b.height), 100);
        QCOMPARE(qRound(b.x), 200); // ancrage en haut à gauche gardé
        QCOMPARE(qRound(b.y), 100);
    }
    void stretchDoesNot()
    {
        const RectF b = box(after(page(), "object.stretch-center", R"({"width":200,"height":50})"));
        QCOMPARE(qRound(b.width), 200);
        QCOMPARE(qRound(b.height), 50);
    }
    void nineAnchors()
    {
        struct Expected
        {
            const char* anchor;
            int x; // coin haut gauche de la boîte (100 × 50) sur la page 1024 × 768
            int y;
        };
        for (const Expected& e :
             {Expected{"top-left", 0, 0}, Expected{"top", 462, 0}, Expected{"top-right", 924, 0},
              Expected{"left", 0, 359}, Expected{"center", 462, 359}, Expected{"right", 924, 359},
              Expected{"bottom-left", 0, 718}, Expected{"bottom", 462, 718},
              Expected{"bottom-right", 924, 718}}) {
            const std::string id = std::string("object.place-") + e.anchor;
            const RectF b = box(after(page(), id.c_str(), "{}"));
            QVERIFY2(qRound(b.x) == e.x && qRound(b.y) == e.y, e.anchor);
        }
    }
    void mirrorIsOneStep()
    {
        Document d = page();
        auto c = objectActionCommand(d, "object.mirror-x", "{}", on(1));
        QVERIFY(c);
        Changes changes;
        c->redo(d, changes);
        // H8-10 : reflet en place, en une étape, par le chemin des transformations (A10) et non par une
        // commande de plus. `-x` vaut Axis::X (haut / bas) comme `transform.mirror-x` et `object.flip-x`.
        QVERIFY(dynamic_cast<const TransformObjectsCommand*>(c.get()));
        QCOMPARE(d.page(0).objects.size(), std::size_t{1});
        QCOMPARE(qRound(box(d).x), 200);
        QCOMPARE(qRound(box(d).y), 100);
        QVERIFY(transformOf(d).m22 < 0);
        QVERIFY(transformOf(d).m11 > 0);
        c->undo(d, changes);
        QCOMPARE(fingerprint(d), fingerprint(page()));
    }
    void fixedRestrictorsAreRespected()
    {
        Document turning = page();
        setTurning(turning.page(0).objects[0].common.restrictions, Turning::Fixed);
        QVERIFY(!objectActionCommand(turning, "object.angle", R"({"angle":30})", on(1)));
        QVERIFY(!objectActionCommand(turning, "object.angle-step", R"({"angle":30})", on(1)));
        QVERIFY(objectActionCommand(turning, "object.place-center", "{}", on(1)));

        Document travel = page();
        setTravel(travel.page(0).objects[0].common.restrictions, Travel::Fixed);
        QVERIFY(!objectActionCommand(travel, "object.place-center", "{}", on(1)));
        QVERIFY(!objectActionCommand(travel, "object.place-step", R"({"dx":10,"dy":0})", on(1)));
        QVERIFY(objectActionCommand(travel, "object.angle", R"({"angle":30})", on(1)));

        Document sizing = page(); // E6 : dimension bloquée
        sizing.page(0).objects[0].common.restrictions.resizable = false;
        QVERIFY(!objectActionCommand(sizing, "object.size-top-left", R"({"width":200})", on(1)));
        QVERIFY(!objectActionCommand(sizing, "object.size-step-center", R"({"factor":2})", on(1)));
        QVERIFY(!objectActionCommand(sizing, "object.stretch-center", R"({"width":200,"height":50})", on(1)));
        QVERIFY(objectActionCommand(sizing, "object.place-center", "{}", on(1)));
    }
    void unknownIdReturnsNull()
    {
        QVERIFY(!objectActionCommand(page(), "object.nothing", "{}", on(1)));
        QVERIFY(!objectActionCommand(page(), "object.angle", "{}", on(99)));
    }
    // Revue de L8-T7.
    void angleIgnoresTheReflection()
    {
        // Objet reflété (gauche / droite) puis « fixer 0° » : rien ne tourne, le reflet reste.
        const Document d = after(after(page(), "object.mirror-y", "{}"), "object.angle", R"({"angle":0})");
        QVERIFY(transformOf(d).m11 < 0);
        QVERIFY(transformOf(d).m22 > 0);
        QCOMPARE(qRound(box(d).x), 200);
        QCOMPARE(qRound(box(d).y), 100);
    }
    void groupAngleReturnsNull()
    {
        Document d = page();
        Group g;
        g.members = {1};
        PageObject group{g};
        group.common.id = 2;
        d.page(0).objects.push_back(group);
        // L'angle d'un groupe n'est pas défini (alignment.cpp) : « fixer » refuse, « tourner de » agit.
        QVERIFY(!objectActionCommand(d, "object.angle", R"({"angle":30})", on(2)));
        QVERIFY(objectActionCommand(d, "object.angle-step", R"({"angle":30})", on(2)));
    }
    void stretchFollowsTheObjectAxes()
    {
        // Tourné de 90° puis étiré à 200 × 50 dans son repère : la boîte de page devient 50 × 200, centre
        // gardé.
        const Document d = after(after(page(), "object.angle-step", R"({"angle":90})"),
                                 "object.stretch-center", R"({"width":200,"height":50})");
        const RectF b = box(d);
        QCOMPARE(qRound(b.width), 50);
        QCOMPARE(qRound(b.height), 200);
        QCOMPARE(qRound(b.x + b.width / 2), 250);
        QCOMPARE(qRound(b.y + b.height / 2), 125);
    }
    void lockedTargetStillActs()
    {
        // H8-11 : le verrou garde contre la main de l'utilisateur, pas contre les actions.
        Document d = page(2048, 100);
        d.page(0).objects[0].common.locked = true;
        QVERIFY(objectActionCommand(d, "object.flip-x", "{}", on(1)));
        QVERIFY(objectActionCommand(d, "object.fit-page", "{}", on(1)));
        QVERIFY(objectActionCommand(d, "object.angle", R"({"angle":30})", on(1)));
        QVERIFY(objectActionCommand(d, "object.place-center", "{}", on(1)));
    }
    void fitLeavesAFixedObjectUnmoved()
    {
        Document d = page(2048, 100);
        setTravel(d.page(0).objects[0].common.restrictions, Travel::Fixed);
        Changes changes;
        auto c = objectActionCommand(d, "object.fit-page", "{}", on(1));
        QVERIFY(c);
        c->redo(d, changes);
        const RectF b = box(d);
        QCOMPARE(qRound(b.width), 1024); // réduit de moitié autour de son centre, sans décalage
        QCOMPARE(qRound(b.x + b.width / 2), 1224);
        QCOMPARE(qRound(b.y + b.height / 2), 150);
    }
};

QTEST_MAIN(TstCoreObjectActions)
#include "tst_core_objectactions.moc"
