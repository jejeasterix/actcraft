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
        QCOMPARE(resolveDrop(p, {2}).outcome, DropOutcome::Free); // ni contenu, ni renvoyé
        // Couche plus basse, même rangée après le panier : derrière quand même (le rang de couche passe avant
        // l'indice).
        Page q = basket();
        q.objects[1].common.layer = Layer::Bottom;
        QVERIFY(!isInFront(q, 2, 1));
        QCOMPARE(resolveDrop(q, {2}).outcome, DropOutcome::Free);
    }
    // Un groupe est rangé au sommet de sa couche, mais seuls ses membres sont dessinés, à leur rang : membres
    // A et B sous le panier, groupe G = {A, B} après lui dans la page → G est derrière le panier.
    void movedGroupJudgedOnItsMembers()
    {
        Page p;
        PageObject a = box(10, {90, 90, 20, 20});
        PageObject b = box(11, {90, 90, 20, 20});
        PageObject c = box(1, {0, 0, 200, 200});
        c.common.container.accepts = Accepts::Anything;
        c.common.container.bounceBack = true;
        PageObject g{Group{{10, 11}}};
        g.common.id = 12;
        p.objects = {a, b, c, g};
        QVERIFY(!isInFront(p, 12, 1));
        QCOMPARE(resolveDrop(p, {12}).outcome, DropOutcome::Free);
        // Le même groupe, membres dessinés devant le panier : contenu.
        p.objects = {c, a, b, g};
        QVERIFY(isInFront(p, 12, 1));
        QCOMPARE(resolveDrop(p, {12}).outcome, DropOutcome::Contained);
    }
    // Miroir : conteneur groupe, membres sous la pomme, groupe rangé après elle → la pomme est devant.
    void groupContainerJudgedOnItsMembers()
    {
        Page p;
        PageObject m1 = box(20, {0, 0, 200, 200});
        PageObject m2 = box(21, {0, 0, 200, 200});
        PageObject apple = box(2, {80, 80, 40, 40});
        PageObject cg{Group{{20, 21}}};
        cg.common.id = 22;
        cg.common.container.accepts = Accepts::Anything;
        p.objects = {m1, m2, apple, cg};
        QVERIFY(isInFront(p, 2, 22));
        const DropResult r = resolveDrop(p, {2});
        QCOMPARE(r.outcome, DropOutcome::Contained);
        QCOMPARE(r.container, ObjectId(22));
        // Pomme dessinée sous les membres : derrière.
        p.objects = {apple, m1, m2, cg};
        QVERIFY(!isInFront(p, 2, 22));
        QCOMPARE(resolveDrop(p, {2}).outcome, DropOutcome::Free);
    }
    // Plusieurs objets déplacés : tous acceptés, sinon refus ; la règle porte sur l'union de leurs emprises.
    void severalMovedAllAccepted()
    {
        Page p = basket();
        QCOMPARE(resolveDrop(p, {2, 3}).outcome, DropOutcome::BounceBack); // la carotte est refusée
        p.objects[2].common.keywords = {"fruit"};
        QCOMPARE(resolveDrop(p, {2, 3}).outcome, DropOutcome::Contained);
        p.objects[2] = box(3, {80 + 30, 80, 40, 40}); // union centrée à 15 px du panier
        p.objects[2].common.keywords = {"fruit"};
        QCOMPARE(resolveDrop(p, {2, 3}).outcome, DropOutcome::BounceBack);
    }
    // Un membre d'un groupe déplacé ne contient pas ce qu'il accompagne.
    void movedGroupMembersAreNotContainers()
    {
        Page p = basket();
        p.objects[0].common.container.accepts = Accepts::Anything;
        PageObject inner = box(6, {60, 60, 80, 80});
        inner.common.container.accepts = Accepts::Anything;
        p.objects.insert(p.objects.begin() + 1, inner);
        PageObject g{Group{{6, 2}}};
        g.common.id = 7;
        p.objects.push_back(g);
        const DropResult r = resolveDrop(p, {7});
        QCOMPARE(r.outcome, DropOutcome::Contained);
        QCOMPARE(r.container, ObjectId(1));
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
        Page p = basket();
        QCOMPARE(resolveDrop(p, {2}).outcome, DropOutcome::Contained);
        QCOMPARE(resolveDrop(p, {2}).container, ObjectId(1));
        QVERIFY(!resolveDrop(p, {2}).chime);
        p.objects[0].common.container.chime = true; // son du gagnant transmis
        QVERIFY(resolveDrop(p, {2}).chime);
        QVERIFY(!resolveDrop(p, {3}).chime); // renvoyé : aucun son
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
    void refusedBouncesBack()
    {
        const DropResult r = resolveDrop(basket(), {3});
        QCOMPARE(r.outcome, DropOutcome::BounceBack);
        QCOMPARE(r.container, ObjectId(1)); // le conteneur qui renvoie
    }
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
    // containedIn ne rend ni un objet dehors, ni un objet derrière, ni le contenu d'un conteneur imbriqué.
    void containedInListsDirectContentsOnly()
    {
        Page p = basket();
        p.objects[0].common.container.accepts = Accepts::Anything;
        p.objects[2] = box(3, {500, 500, 40, 40}); // carotte dehors
        QCOMPARE(containedIn(p, 1), std::vector<ObjectId>{2});
        std::swap(p.objects[0], p.objects[1]); // pomme derrière le panier
        QVERIFY(containedIn(p, 1).empty());
        Page q = basket();
        PageObject inner = box(4, {60, 60, 80, 80});
        inner.common.container.accepts = Accepts::Anything;
        q.objects.insert(q.objects.begin() + 1, inner);
        QVERIFY(
            containedIn(q, 1).empty()); // pomme et carotte dans l'imbriqué, lui-même refusé (sans mot-clé)
        QCOMPARE(containedIn(q, 4), (std::vector<ObjectId>{2, 3}));
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
