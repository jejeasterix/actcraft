#include <actcraft/core/containers.h>

#include <QtTest>

#include <algorithm>
#include <utility>

using namespace actcraft::core;

class TstCoreContainers : public QObject
{
    Q_OBJECT
    static PageObject box(ObjectId id, RectF r, Layer layer = Layer::Middle)
    {
        Shape s;
        s.bounds = r;
        PageObject o{s};
        o.common.id = id;
        o.common.layer = layer;
        return o;
    }
    // Panier 1 (0,0,200,200) accepte « fruit » ; pomme 2 et carotte 3 de 40 × 40.
    static Page basket(bool bounce = true)
    {
        Page p;
        PageObject b = box(1, {0, 0, 200, 200});
        b.common.container.accepts = Accepts::Keywords;
        b.common.container.keywords = {"fruit"};
        b.common.container.bounceBack = bounce;
        PageObject apple = box(2, {80, 80, 40, 40});
        apple.common.keywords = {"fruit"};
        PageObject carrot = box(3, {80, 80, 40, 40});
        carrot.common.keywords = {"légume"};
        p.objects = {b, apple, carrot};
        return p;
    }
private slots:
    void anythingAcceptsInFront()
    {
        Page p = basket();
        p.objects[0].common.container.accepts = Accepts::Anything;
        QCOMPARE(resolveDrop(p, {3}).outcome, DropOutcome::Contained);
    }
    // Le test qui distingue : centres confondus, mot-clé accepté, seule la superposition dit non. Une règle
    // purement géométrique (ou un objectAt au point de dépôt) rendrait Contained.
    void behindIsNeverContained()
    {
        Page p = basket();
        std::swap(p.objects[0], p.objects[1]); // la pomme passe derrière le panier
        QVERIFY(!isInFront(p, 2, 1));
        QVERIFY(resolveDrop(p, {2}).outcome != DropOutcome::Contained);
        // Couche plus basse, même rangée après le panier : derrière quand même (le rang de couche passe avant
        // l'indice).
        Page q = basket();
        q.objects[1].common.layer = Layer::Bottom;
        QVERIFY(!isInFront(q, 2, 1));
        QVERIFY(resolveDrop(q, {2}).outcome != DropOutcome::Contained);
    }
    void oneObjectOnly()
    {
        Page p = basket();
        p.objects[0].common.container.accepts = Accepts::OneObject;
        p.objects[0].common.container.object = 3;
        QCOMPARE(resolveDrop(p, {3}).outcome, DropOutcome::Contained);
        QCOMPARE(resolveDrop(p, {2}).outcome, DropOutcome::BounceBack);
    }
    void commonKeyword()
    {
        const Page p = basket();
        QCOMPARE(resolveDrop(p, {2}).outcome, DropOutcome::Contained);
        QCOMPARE(resolveDrop(p, {2}).container, ObjectId(1));
    }
    void centresWithinTolerance()
    {
        Page p = basket();
        p.objects[1] = box(2, {80 + 9, 80, 40, 40}); // centre à 9 px du centre du panier
        p.objects[1].common.keywords = {"fruit"};
        QCOMPARE(resolveDrop(p, {2}).outcome, DropOutcome::Contained);
        p.objects[1] = box(2, {80 + 11, 80, 40, 40});
        p.objects[1].common.keywords = {"fruit"};
        QCOMPARE(resolveDrop(p, {2}).outcome, DropOutcome::BounceBack);
    }
    void fullyInside()
    {
        Page p = basket();
        p.objects[0].common.container.fit = FitRule::FullyInside;
        p.objects[1] = box(2, {150, 150, 40, 40});
        p.objects[1].common.keywords = {"fruit"};
        QCOMPARE(resolveDrop(p, {2}).outcome, DropOutcome::Contained);
        p.objects[1] = box(2, {180, 180, 40, 40});
        p.objects[1].common.keywords = {"fruit"};
        QCOMPARE(resolveDrop(p, {2}).outcome, DropOutcome::BounceBack);
    }
    void refusedBouncesBack() { QCOMPARE(resolveDrop(basket(), {3}).outcome, DropOutcome::BounceBack); }
    void refusedWithoutBounceStays() { QCOMPARE(resolveDrop(basket(false), {3}).outcome, DropOutcome::Free); }
    void nestedTopmostWins()
    {
        Page p = basket();
        PageObject inner = box(4, {60, 60, 80, 80});
        inner.common.container.accepts = Accepts::Anything;
        p.objects.insert(p.objects.begin() + 1, inner); // au-dessus du panier, sous la pomme
        QCOMPARE(resolveDrop(p, {2}).container, ObjectId(4));
    }
    void strokeDrawnInsideIsContained()
    {
        Page p = basket();
        p.objects[0].common.container.accepts = Accepts::Anything;
        Stroke s;
        s.points = {{90, 90, 0.5f}, {110, 110, 0.5f}};
        PageObject o{s};
        o.common.id = 5;
        o.common.layer = Layer::Top;
        p.objects.push_back(o);
        const auto inside = containedIn(p, 1);
        QVERIFY(std::find(inside.begin(), inside.end(), ObjectId(5)) != inside.end());
    }
    void leavingIsOneGesture()
    {
        Page p = basket();
        p.objects[1] = box(2, {500, 500, 40, 40}); // pomme sortie du panier en un glisser
        p.objects[1].common.keywords = {"fruit"};
        QCOMPARE(resolveDrop(p, {2}).outcome, DropOutcome::Free);
    }
    void hiddenContainerIgnored()
    {
        Page p = basket();
        p.objects[0].common.visible = false;
        QCOMPARE(resolveDrop(p, {3}).outcome, DropOutcome::Free);
    }
};

QTEST_MAIN(TstCoreContainers)
#include "tst_core_containers.moc"
