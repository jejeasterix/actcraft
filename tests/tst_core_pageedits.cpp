#include <actcraft/core/document.h>
#include <actcraft/core/pagecache.h>
#include <actcraft/core/pageedits.h>
#include <actcraft/core/undo.h>

#include <QtTest>

#include <limits>
#include <string>

using namespace actcraft::core;

namespace {
Document pages(int n)
{
    Document d;
    for (int i = 1; i < n; ++i)
        d.appendPage();
    for (std::size_t i = 0; i < d.pageCount(); ++i)
        d.page(i).name = std::to_string(i + 1);
    return d;
}
std::string names(const Document& d)
{
    std::string s;
    for (std::size_t i = 0; i < d.pageCount(); ++i)
        s += d.page(i).name;
    return s;
}
PageObject shape(ObjectId id)
{
    PageObject o{Shape{}};
    o.common.id = id;
    return o;
}
struct Spy final : PageCache
{
    int calls = 0;
    void insert(ObjectId) override { ++calls; }
    void remove(ObjectId, const RectF&) override { ++calls; }
    void invalidate(ObjectId, const CacheArea&) override { ++calls; }
};
} // namespace

// Les tests qui gardent le contrat d'annulation et du cache (une étape, ordre exact, page affichée suivie
// sans redessin) distinguent ces commandes d'un patron qui reconstruirait toutes les pages : celui-ci
// rendrait un PageChanged par page (redessin de la page affichée, indice non suivi) ou retirerait la page
// affichée (applyToCache rend std::nullopt).
class TstCorePageEdits : public QObject
{
    Q_OBJECT

private slots:
    void moveTwoPagesToFront()
    {
        Document d = pages(4);
        d.page(2).objects = {shape(7)};
        d.refreshNextId();
        const ObjectId next = d.nextId();
        UndoStack stack;
        stack.push(d, std::make_unique<MovePagesCommand>(std::vector<std::size_t>{2, 3}, 0));
        QCOMPARE(QString::fromStdString(names(d)), QStringLiteral("3412"));
        QCOMPARE(positionsAfterMove(4, {2, 3}, 0), (std::vector<std::size_t>{0, 1}));
        // Déplacer n'est pas coller une copie : mêmes objets, aucun identifiant tiré, une seule étape.
        QCOMPARE(d.page(0).objects[0].common.id, ObjectId{7});
        QCOMPARE(d.nextId(), next);
        QCOMPARE(stack.count(), std::size_t{1});
        stack.undo(d);
        QCOMPARE(QString::fromStdString(names(d)), QStringLiteral("1234"));
        QCOMPARE(d.page(2).objects[0].common.id, ObjectId{7});
    }
    void moveWithoutEffectLeavesTheOrder()
    {
        Document d = pages(3);
        UndoStack stack;
        const Changes c = stack.push(d, std::make_unique<MovePagesCommand>(std::vector<std::size_t>{1}, 1));
        QVERIFY(c.empty());
        QCOMPARE(QString::fromStdString(names(d)), QStringLiteral("123"));
        QVERIFY(stack.undo(d).empty());
        QCOMPARE(QString::fromStdString(names(d)), QStringLiteral("123"));
    }
    void undoRestoresTheExactOrder()
    {
        // Sélection non contiguë : les pages 1, 3 et 6 passent, dans leur ordre, devant la page 5.
        Document d = pages(6);
        UndoStack stack;
        stack.push(d, std::make_unique<MovePagesCommand>(std::vector<std::size_t>{0, 2, 5}, 4));
        QCOMPARE(QString::fromStdString(names(d)), QStringLiteral("241365"));
        QCOMPARE(positionsAfterMove(6, {0, 2, 5}, 4), (std::vector<std::size_t>{2, 3, 4}));
        QCOMPARE(stack.count(), std::size_t{1});
        stack.undo(d);
        QCOMPARE(QString::fromStdString(names(d)), QStringLiteral("123456"));
        stack.redo(d);
        QCOMPARE(QString::fromStdString(names(d)), QStringLiteral("241365"));
        stack.undo(d);
        QCOMPARE(QString::fromStdString(names(d)), QStringLiteral("123456"));
    }
    void removeRefusesAllPages()
    {
        Document d = pages(2);
        UndoStack stack;
        const Changes refused = stack.push(d, std::make_unique<RemovePagesCommand>(std::vector<std::size_t>{0, 1}));
        QVERIFY(refused.empty());
        QCOMPARE(d.pageCount(), std::size_t{2});
        stack.push(d, std::make_unique<RemovePagesCommand>(std::vector<std::size_t>{0}));
        QCOMPARE(QString::fromStdString(names(d)), QStringLiteral("2"));
        stack.undo(d);
        QCOMPARE(QString::fromStdString(names(d)), QStringLiteral("12"));
    }
    void removeFollowsTheShownPage()
    {
        // Page « 4 » affichée (indice 3) ; les pages 1 et 3 retirées : elle passe à l'indice 1, sans redessin.
        // Retirées dans l'ordre croissant à leurs indices d'origine, la seconde viserait l'indice affiché.
        Document d = pages(5);
        UndoStack stack;
        const Changes c = stack.push(d, std::make_unique<RemovePagesCommand>(std::vector<std::size_t>{0, 2}));
        QCOMPARE(QString::fromStdString(names(d)), QStringLiteral("245"));
        Spy spy;
        const auto shown = applyToCache(c, 3, spy);
        QVERIFY(shown.has_value());
        QCOMPARE(*shown, std::size_t{1});
        const Changes back = stack.undo(d);
        QCOMPARE(QString::fromStdString(names(d)), QStringLiteral("12345"));
        const auto again = applyToCache(back, 1, spy);
        QVERIFY(again.has_value());
        QCOMPARE(*again, std::size_t{3});
        QCOMPARE(spy.calls, 0);
    }
    void insertGivesFreshIds()
    {
        Document d = pages(1);
        d.page(0).objects = {shape(1), shape(2)};
        d.refreshNextId();
        UndoStack stack;
        stack.push(d, std::make_unique<InsertPagesCommand>(1, std::vector<Page>{d.page(0)}));
        QCOMPARE(d.pageCount(), std::size_t{2});
        QVERIFY(hasUniqueIds(d));
        QCOMPARE(d.page(0).objects[0].common.id, ObjectId{1}); // l'original garde les siens
        QCOMPARE(d.page(0).objects[1].common.id, ObjectId{2});
    }
    void insertMovesTheShownPage()
    {
        // Deux copies collées devant la page affichée (« 2 », indice 1) : elle recule de deux rangs, en une étape.
        Document d = pages(3);
        UndoStack stack;
        const Changes c =
            stack.push(d, std::make_unique<InsertPagesCommand>(1, std::vector<Page>{d.page(2), d.page(2)}));
        QCOMPARE(QString::fromStdString(names(d)), QStringLiteral("13323"));
        QCOMPARE(stack.count(), std::size_t{1});
        Spy spy;
        const auto shown = applyToCache(c, 1, spy);
        QVERIFY(shown.has_value());
        QCOMPARE(*shown, std::size_t{3});
        const Changes back = stack.undo(d);
        QCOMPARE(QString::fromStdString(names(d)), QStringLiteral("123"));
        const auto again = applyToCache(back, 3, spy);
        QVERIFY(again.has_value());
        QCOMPARE(*again, std::size_t{1});
        QCOMPARE(spy.calls, 0);
    }
    void groupMembersFollowFreshIds()
    {
        Document d = pages(1);
        PageObject group{Group{{1, 2}}};
        group.common.id = 3;
        d.page(0).objects = {shape(1), shape(2), group};
        d.refreshNextId();
        const Page copy = withFreshIds(d, d.page(0));
        QVERIFY(copy.objects[0].common.id != ObjectId{1}); // renumérotée
        QVERIFY(copy.objects[1].common.id != ObjectId{2});
        const auto& members = std::get<Group>(body(copy.objects[2])).members;
        QCOMPARE(members[0], copy.objects[0].common.id);
        QCOMPARE(members[1], copy.objects[1].common.id);
    }
    void connectorEndsFollow()
    {
        Document d = pages(1);
        Connector link;
        link.objectA = 1;
        link.objectB = 99; // hors de la page : gardé
        PageObject c{link};
        c.common.id = 2;
        d.page(0).objects = {shape(1), c};
        d.reserveIdsBelow(100);
        const Page copy = withFreshIds(d, d.page(0));
        QVERIFY(copy.objects[0].common.id != ObjectId{1});
        QCOMPARE(std::get<Connector>(body(copy.objects[1])).objectA, copy.objects[0].common.id);
        QCOMPARE(std::get<Connector>(body(copy.objects[1])).objectB, ObjectId{99});
    }
    void actionTargetsFollow()
    {
        Document d = pages(1);
        PageObject a = shape(2);
        Action action;
        action.target = 1;
        a.common.actions.push_back(action);
        d.page(0).objects = {shape(1), a};
        d.refreshNextId();
        const Page copy = withFreshIds(d, d.page(0));
        QVERIFY(copy.objects[0].common.id != ObjectId{1});
        QCOMPARE(copy.objects[1].common.actions[0].target, copy.objects[0].common.id);
    }
    void emptyReferencesStayEmpty()
    {
        // Un objet sans identifiant (0) en reçoit un ; une référence à 0 (« aucun ») ne le vise pas.
        Document d = pages(1);
        Connector link; // objectA = objectB = 0
        PageObject c{link};
        c.common.id = 5;
        Page page;
        page.objects = {shape(0), c};
        d.reserveIdsBelow(10);
        const Page copy = withFreshIds(d, page);
        QVERIFY(copy.objects[0].common.id != ObjectId{0});
        QCOMPARE(std::get<Connector>(body(copy.objects[1])).objectA, ObjectId{0});
        QCOMPARE(std::get<Connector>(body(copy.objects[1])).objectB, ObjectId{0});
    }
    void redoKeepsTheSameIds()
    {
        Document d = pages(1);
        d.page(0).objects = {shape(1)};
        d.refreshNextId();
        UndoStack stack;
        stack.push(d, std::make_unique<InsertPagesCommand>(1, std::vector<Page>{d.page(0)}));
        const ObjectId first = d.page(1).objects[0].common.id;
        QVERIFY(first != ObjectId{1});
        stack.undo(d);
        QCOMPARE(d.pageCount(), std::size_t{1});
        stack.redo(d);
        QCOMPARE(d.page(1).objects[0].common.id, first);
        QVERIFY(hasUniqueIds(d));
    }
    void exhaustedIdsInsertNothing()
    {
        // Comme AddObjectCommand : identifiants épuisés, aucune page insérée, aucun changement rendu.
        Document d = pages(1);
        d.page(0).objects = {shape(1)};
        d.reserveIdsBelow(std::numeric_limits<ObjectId>::max());
        UndoStack stack;
        QTest::ignoreMessage(QtWarningMsg, "Document : identifiants d'objets épuisés");
        const Changes c = stack.push(d, std::make_unique<InsertPagesCommand>(1, std::vector<Page>{d.page(0)}));
        QVERIFY(c.empty());
        QCOMPARE(d.pageCount(), std::size_t{1});
        QVERIFY(stack.undo(d).empty());
        QCOMPARE(d.pageCount(), std::size_t{1});
    }
    void cacheFollowsTheShownPage()
    {
        Document d = pages(3);
        UndoStack stack;
        const Changes c = stack.push(d, std::make_unique<MovePagesCommand>(std::vector<std::size_t>{2}, 0));
        Spy spy;
        const auto shown = applyToCache(c, 1, spy); // la page « 2 » affichée passe à l'indice 2
        QVERIFY(shown.has_value());
        QCOMPARE(*shown, std::size_t{2});
        // L'annulation la ramène à l'indice 1 ; ni l'une ni l'autre ne redessine la page affichée.
        const Changes back = stack.undo(d);
        const auto again = applyToCache(back, 2, spy);
        QVERIFY(again.has_value());
        QCOMPARE(*again, std::size_t{1});
        QCOMPARE(spy.calls, 0);
    }
};

QTEST_GUILESS_MAIN(TstCorePageEdits)
#include "tst_core_pageedits.moc"
