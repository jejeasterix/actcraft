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
    static Document page()
    {
        Document d(1024, 768);
        Shape s;
        s.bounds = {0, 0, 100, 50};
        s.outlineWidth = 0; // boundsOnPage compte l'épaisseur (edits.h) : boîte = 100 × 50 exactement

        PageObject o{s};
        o.common.id = 1;
        o.common.transform = translated(200, 100);
        d.page(0).objects.push_back(o);
        return d;
    }
    static Document after(Document d, const char* id, const char* params)
    {
        auto c = objectActionCommand(d, id, params, ActionContext{0, 1, 0, 1024, 768});
        [&] { QVERIFY2(c, id); }();
        Changes changes;
        c->redo(d, changes);
        return d;
    }
    static RectF box(const Document& d) { return boundsOnPage(d.page(0).objects[0]); }
    static double angleOf(const Document& d) { return rotationOf(d.page(0).objects[0].common.transform); }
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
    }
    void stretchDoesNot()
    {
        const RectF b = box(after(page(), "object.stretch-center", R"({"width":200,"height":50})"));
        QCOMPARE(qRound(b.width), 200);
        QCOMPARE(qRound(b.height), 50);
    }
    void nineAnchors()
    {
        for (const char* a : {"top-left", "top", "top-right", "left", "center", "right", "bottom-left",
                              "bottom", "bottom-right"}) {
            const std::string id = std::string("object.place-") + a;
            QVERIFY2(objectActionCommand(page(), id, "{}", ActionContext{0, 1, 0, 1024, 768}), a);
        }
    }
    void mirrorIsOneStep()
    {
        Document d = page();
        auto c = objectActionCommand(d, "object.mirror-x", "{}", ActionContext{0, 1, 0, 1024, 768});
        QVERIFY(c);
        Changes changes;
        c->redo(d, changes);
        // Reflet en place, en une étape, par le chemin des transformations (A10) et non par une commande de
        // plus : la boîte ne bouge pas, la largeur est renversée.
        QVERIFY(dynamic_cast<const TransformObjectsCommand*>(c.get()));
        QCOMPARE(d.page(0).objects.size(), std::size_t{1});
        QCOMPARE(qRound(box(d).x), 200);
        QVERIFY(d.page(0).objects[0].common.transform.m11 < 0);
        c->undo(d, changes);
        QCOMPARE(fingerprint(d), fingerprint(page()));
    }
    void fixedRestrictorsAreRespected()
    {
        Document d = page();
        setTurning(d.page(0).objects[0].common.restrictions, Turning::Fixed);
        setTravel(d.page(0).objects[0].common.restrictions, Travel::Fixed);
        QVERIFY(
            !objectActionCommand(d, "object.angle", R"({"angle":30})", ActionContext{0, 1, 0, 1024, 768}));
        QVERIFY(!objectActionCommand(d, "object.place-center", "{}", ActionContext{0, 1, 0, 1024, 768}));
    }
    void unknownIdReturnsNull()
    {
        QVERIFY(!objectActionCommand(page(), "object.nothing", "{}", ActionContext{0, 1, 0, 1024, 768}));
        QVERIFY(!objectActionCommand(page(), "object.angle", "{}", ActionContext{0, 99, 0, 1024, 768}));
    }
};

QTEST_MAIN(TstCoreObjectActions)
#include "tst_core_objectactions.moc"
