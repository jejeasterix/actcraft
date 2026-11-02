#include <actcraft/core/arrange.h>
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/undo.h>

#include <QTest>

#include <algorithm>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace core = actcraft::core;

namespace {

// Page : a, b, c, d sur la couche du milieu dans cet ordre, e sur celle du dessus. Identifiants 1 à 5.
core::Document pageABCDE()
{
    core::Document d;
    const char* names = "abcde";
    for (int i = 0; i < 5; ++i) {
        core::Shape s;
        s.bounds = {10.0 * i, 10.0, 8.0, 8.0};
        core::PageObject o = s;
        o.common.id = static_cast<core::ObjectId>(i + 1);
        o.common.name = std::string(1, names[i]);
        o.common.layer = (i == 4) ? core::Layer::Top : core::Layer::Middle;
        d.page(0).objects.push_back(std::move(o));
    }
    d.refreshNextId();
    return d;
}

core::ObjectId id(const core::Document&, char c)
{
    return static_cast<core::ObjectId>(c - 'a' + 1);
}

std::vector<core::ObjectId> ids(const core::Document& d, const char* letters)
{
    std::vector<core::ObjectId> out;
    for (const char* p = letters; *p != '\0'; ++p) {
        out.push_back(id(d, *p));
    }
    return out;
}

std::vector<core::ObjectId> ordered(const core::Document& d)
{
    std::vector<core::ObjectId> out;
    for (const auto& o : d.page(0).objects) {
        out.push_back(o.common.id);
    }
    return out;
}

core::PageObject& objectById(core::Document& d, core::ObjectId target)
{
    auto& page = d.page(0);
    return page.objects[core::indexOf(page, target)];
}

bool hasObject(const core::Document& d, core::ObjectId target)
{
    return core::indexOf(d.page(0), target) < d.page(0).objects.size();
}

int countKind(const core::Changes& changes, core::Change::Kind kind)
{
    return static_cast<int>(
        std::count_if(changes.begin(), changes.end(), [kind](const auto& c) { return c.kind == kind; }));
}

} // namespace

class TestArrange : public QObject
{
    Q_OBJECT
private slots:
    void forwardSwapsOneStep()
    {
        auto d = pageABCDE();
        QCOMPARE(core::arrangedOrder(d.page(0), {id(d, 'b')}, core::ArrangeMove::Forward), ids(d, "acbde"));
    }
    void backwardSwapsOneStep()
    {
        auto d = pageABCDE();
        QCOMPARE(core::arrangedOrder(d.page(0), {id(d, 'c')}, core::ArrangeMove::Backward), ids(d, "acbde"));
        // En bas de sa couche : ne passe pas sous une autre couche, ordre inchangé.
        QCOMPARE(core::arrangedOrder(d.page(0), {id(d, 'a')}, core::ArrangeMove::Backward), ids(d, "abcde"));
        // Tout en haut de sa couche : forward ne bouge pas non plus.
        QCOMPARE(core::arrangedOrder(d.page(0), {id(d, 'd')}, core::ArrangeMove::Forward), ids(d, "abcde"));
    }
    void frontKeepsRelativeOrder()
    {
        auto d = pageABCDE();
        QCOMPARE(core::arrangedOrder(d.page(0), {id(d, 'a'), id(d, 'b')}, core::ArrangeMove::Front),
                 ids(d, "cdabe"));
        QCOMPARE(core::arrangedOrder(d.page(0), {id(d, 'd')}, core::ArrangeMove::Back), ids(d, "dabce"));
    }
    void layerChangeLandsAboveTheOthers()
    {
        auto d = pageABCDE();
        core::UndoStack stack;
        stack.push(d, std::make_unique<core::SetLayerCommand>(0, std::vector{id(d, 'a')}, core::Layer::Top));
        QCOMPARE(objectById(d, id(d, 'a')).common.layer, core::Layer::Top);
        QCOMPARE(d.page(0).objects.back().common.id, id(d, 'a')); // au sommet de la couche du dessus
        QVERIFY(core::isLayerOrdered(d.page(0)));
        stack.undo(d);
        QCOMPARE(ordered(d), ids(d, "abcde"));
        QCOMPARE(objectById(d, id(d, 'a')).common.layer, core::Layer::Middle);
    }
    void groupingIsASingleStep()
    {
        auto d = pageABCDE();
        core::UndoStack stack;
        stack.push(d, core::makeGroupCommand(d.page(0), 0, {id(d, 'a'), id(d, 'e')}));
        // Membres sur deux couches : le groupe prend la plus basse (D4) et va au sommet de celle-ci (A6),
        // donc sous e, qui est sur la couche du dessus.
        QCOMPARE(d.page(0).objects.size(), std::size_t(6));
        const auto& g = d.page(0).objects[4];
        QVERIFY(std::holds_alternative<core::Group>(g));
        QCOMPARE(std::get<core::Group>(g).members.size(), std::size_t(2));
        QCOMPARE(g.common.layer, core::Layer::Middle);
        QCOMPARE(d.page(0).objects.back().common.id, id(d, 'e'));
        QCOMPARE(core::topLevelGroupOf(d.page(0), id(d, 'a')), g.common.id);
        QCOMPARE(core::topLevelGroupOf(d.page(0), id(d, 'c')), core::ObjectId(0));
        stack.undo(d);
        QCOMPARE(d.page(0).objects.size(), std::size_t(5));
        QCOMPARE(ordered(d), ids(d, "abcde"));
        QVERIFY(core::makeGroupCommand(d.page(0), 0, {id(d, 'a')}) == nullptr);
        QVERIFY(core::makeGroupCommand(d.page(0), 0, {id(d, 'a'), id(d, 'a')}) == nullptr);
        QVERIFY(core::makeGroupCommand(d.page(0), 0, {id(d, 'a'), 99}) == nullptr);
    }
    void ungroupRestoresInnerGroups()
    {
        auto d = pageABCDE();
        core::UndoStack stack;
        stack.push(d, core::makeGroupCommand(d.page(0), 0, {id(d, 'a'), id(d, 'b')}));
        const auto inner = d.page(0).objects[d.page(0).objects.size() - 2].common.id; // sous e
        stack.push(d, core::makeGroupCommand(d.page(0), 0, {inner, id(d, 'c')}));
        const auto outer = d.page(0).objects[d.page(0).objects.size() - 2].common.id;
        QCOMPARE(core::topLevelGroupOf(d.page(0), id(d, 'a')), outer);
        QCOMPARE(core::expandGroups(d.page(0), {outer}),
                 (std::vector{id(d, 'c'), id(d, 'a'), id(d, 'b'), inner, outer})); // membres en ordre de page
        stack.push(d, core::makeUngroupCommand(d.page(0), 0, {outer}));
        QVERIFY(hasObject(d, inner));
        QVERIFY(!hasObject(d, outer));
        QCOMPARE(core::expandGroups(d.page(0), {inner}), (std::vector{id(d, 'a'), id(d, 'b'), inner}));
        QCOMPARE(core::topLevelGroupOf(d.page(0), id(d, 'a')), inner);
        QVERIFY(core::makeUngroupCommand(d.page(0), 0, {id(d, 'a')}) == nullptr); // pas un groupe
        stack.undo(d);
        QVERIFY(hasObject(d, outer));
    }
    void expandGroupsSurvivesCycles()
    {
        auto d = pageABCDE();
        core::Group g1;
        g1.members = {7};
        core::Group g2;
        g2.members = {6};
        core::PageObject o1 = g1;
        o1.common.id = 6;
        core::PageObject o2 = g2;
        o2.common.id = 7;
        d.page(0).objects.push_back(o1); // fichier lu d'ailleurs : cycle 6 -> 7 -> 6 ; ordre sans importance ici
        d.page(0).objects.push_back(o2);
        QCOMPARE(core::expandGroups(d.page(0), {6}).size(), std::size_t(2));
        static_cast<void>(core::topLevelGroupOf(d.page(0), 6)); // doit rendre la main
    }
    void groupMovesWithItsMembers()
    {
        auto d = pageABCDE();
        core::UndoStack stack;
        stack.push(d, core::makeGroupCommand(d.page(0), 0, {id(d, 'a'), id(d, 'b')}));
        const auto g = d.page(0).objects[4].common.id; // a b c d G e
        // Le groupe passe au premier plan de sa couche : ses membres le suivent.
        QCOMPARE(core::arrangedOrder(d.page(0), {g}, core::ArrangeMove::Back),
                 (std::vector{id(d, 'a'), id(d, 'b'), g, id(d, 'c'), id(d, 'd'), id(d, 'e')}));
    }
    void reorderChangesSeenByCache()
    {
        auto d = pageABCDE();
        core::Changes changes;
        core::ReorderObjectsCommand command(0, ids(d, "bacde"));
        command.redo(d, changes);
        QCOMPARE(ordered(d), ids(d, "bacde"));
        // a et b changent de rang : un retrait puis une insertion chacun, rien pour c, d, e.
        QCOMPARE(countKind(changes, core::Change::Kind::ObjectRemoved), 2);
        QCOMPARE(countKind(changes, core::Change::Kind::ObjectInserted), 2);
        core::Changes back;
        command.undo(d, back);
        QCOMPARE(ordered(d), ids(d, "abcde"));
        QCOMPARE(countKind(back, core::Change::Kind::ObjectRemoved), 2);
        core::Changes same;
        core::ReorderObjectsCommand identity(0, ids(d, "abcde"));
        identity.redo(d, same);
        QVERIFY(same.empty());
    }
};

QTEST_GUILESS_MAIN(TestArrange)
#include "tst_core_arrange.moc"
