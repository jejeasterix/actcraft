#include <actcraft/core/clipboardedits.h>
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <QTest>

#include <algorithm>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

namespace core = actcraft::core;

namespace {

// Document de test : une page.
core::Document blank()
{
    return core::Document(std::vector<core::Page>(1));
}

core::ObjectId add(core::Document& d, core::PageObject object, core::Layer layer)
{
    object.common.id = d.allocateId();
    object.common.layer = layer;
    const core::ObjectId id = object.common.id;
    core::insertAtTopOfLayer(d.page(0), std::move(object));
    return id;
}

core::ObjectId addRect(core::Document& d, core::RectF bounds, core::Layer layer = core::Layer::Middle)
{
    core::Shape shape;
    shape.kind = core::ShapeKind::Rectangle;
    shape.bounds = bounds;
    core::PageObject object = shape;
    return add(d, std::move(object), layer);
}

core::ObjectId addStroke(core::Document& d)
{
    core::PageObject object = core::Stroke{};
    return add(d, std::move(object), core::Layer::Middle);
}

core::ObjectId addGroup(core::Document& d, std::vector<core::ObjectId> members)
{
    core::Group group;
    group.members = std::move(members);
    core::PageObject object = group;
    return add(d, std::move(object), core::Layer::Middle);
}

core::ObjectId addConnector(core::Document& d, core::ObjectId a, core::ObjectId b)
{
    core::Connector link;
    link.firstEnd = {5, 5};
    link.secondEnd = {105, 5};
    link.objectA = a;
    link.objectB = b;
    core::PageObject object = link;
    return add(d, std::move(object), core::Layer::Middle);
}

bool hasObject(const core::Document& d, core::ObjectId id)
{
    return id != 0 && core::indexOf(d.page(0), id) < d.page(0).objects.size();
}

core::PageObject& objectById(core::Document& d, core::ObjectId id)
{
    return d.page(0).objects[core::indexOf(d.page(0), id)];
}

core::RectF boundsOf(const core::PageObject& object)
{
    return core::boundsOnPage(object);
}

const core::Group& asGroup(const core::PageObject& object)
{
    return std::get<core::Group>(core::body(object));
}

const core::Connector& asConnector(const core::PageObject& object)
{
    return std::get<core::Connector>(core::body(object));
}

std::size_t insertedCount(const core::Changes& changes)
{
    return static_cast<std::size_t>(std::count_if(changes.begin(), changes.end(), [](const core::Change& c) {
        return c.kind == core::Change::Kind::ObjectInserted;
    }));
}

} // namespace

class TestClipboardEdits : public QObject
{
    Q_OBJECT
private slots:
    void pasteOffsetsBy20();
    void duplicateOffsetsBy10AndSelectsCopy();
    void copiesAreUnlockedAndUnnamed();
    void groupReferencesRemapped();
    void nestedGroupsRemapped();
    void connectorEndsRemapped();
    void pasteOnObjectKeepsItsLayer();
    void cutIsOneStep();
};

void TestClipboardEdits::pasteOffsetsBy20()
{
    core::Document d = blank();
    const auto a = addRect(d, {100, 100, 50, 50});
    auto prepared =
        core::preparePaste(core::closureForCopy(d.page(0), {a}), core::kPasteOffset, core::kPasteOffset);
    QCOMPARE(prepared.size(), std::size_t(1));
    core::UndoStack stack;
    stack.push(d, std::make_unique<core::PasteObjectsCommand>(0, std::move(prepared)));
    QCOMPARE(d.page(0).objects.size(), std::size_t(2));
    // boundsOnPage compte le demi-trait : le décalage se mesure contre l'original.
    QCOMPARE(boundsOf(d.page(0).objects[1]).x - boundsOf(objectById(d, a)).x, 20.0);
    QCOMPARE(boundsOf(d.page(0).objects[1]).y - boundsOf(objectById(d, a)).y, 20.0);
    QCOMPARE(boundsOf(objectById(d, a)).x, 99.5); // l'original ne bouge pas (100 moins le demi-trait)
    stack.undo(d);
    QCOMPARE(d.page(0).objects.size(), std::size_t(1));
}

void TestClipboardEdits::duplicateOffsetsBy10AndSelectsCopy()
{
    core::Document d = blank();
    const auto a = addRect(d, {100, 100, 50, 50});
    auto command = core::makeDuplicateCommand(d.page(0), 0, {a});
    QVERIFY(command != nullptr);
    core::PasteObjectsCommand* raw = command.get();
    core::UndoStack stack;
    stack.push(d, std::move(command));
    QCOMPARE(raw->addedIds().size(), std::size_t(1));
    QVERIFY(raw->addedIds()[0] != a);
    QVERIFY(hasObject(d, raw->addedIds()[0]));
    QCOMPARE(boundsOf(objectById(d, raw->addedIds()[0])).x - boundsOf(objectById(d, a)).x, 10.0);
    QCOMPARE(boundsOf(objectById(d, raw->addedIds()[0])).y - boundsOf(objectById(d, a)).y, 10.0);
}

void TestClipboardEdits::copiesAreUnlockedAndUnnamed()
{
    core::Document d = blank();
    const auto a = addRect(d, {0, 0, 10, 10});
    objectById(d, a).common.locked = true;
    objectById(d, a).common.name = "label";
    const auto copies = core::preparePaste(core::closureForCopy(d.page(0), {a}), 0, 0);
    QCOMPARE(copies.size(), std::size_t(1));
    QVERIFY(!copies[0].common.locked);
    QVERIFY(copies[0].common.name.empty());
    QCOMPARE(copies[0].common.id, core::ObjectId(1)); // rang provisoire, pas l'identifiant d'origine
    QVERIFY(objectById(d, a).common.locked);          // l'original garde son verrou et son nom
    QCOMPARE(objectById(d, a).common.name, std::string("label"));
}

// Test qui distingue les patrons : ReplaceObjectsCommand tire ses identifiants au premier redo() et ignore
// ceux que portent les ajouts. Des identifiants neufs posés avant l'ajout laisseraient les membres pointer
// vers des identifiants absents (et nextId() avancerait de 6) ; des identifiants gardés ou des rangs non
// réécrits pointeraient vers a et b.
void TestClipboardEdits::groupReferencesRemapped()
{
    core::Document d = blank();
    addRect(d, {500, 500, 10, 10}); // leurre : identifiants d'origine distincts des rangs provisoires
    const auto a = addRect(d, {0, 0, 10, 10});
    const auto b = addRect(d, {20, 0, 10, 10});
    const auto g = addGroup(d, {a, b});
    const core::ObjectId nextBefore = d.nextId();
    auto prepared = core::preparePaste(core::closureForCopy(d.page(0), {g}), 20, 20);
    QCOMPARE(prepared.size(), std::size_t(3));
    auto command = std::make_unique<core::PasteObjectsCommand>(0, std::move(prepared));
    core::PasteObjectsCommand* raw = command.get();
    core::UndoStack stack;
    const core::Changes changes = stack.push(d, std::move(command));

    // Une étape, aucun Change de plus que les insertions, aucun identifiant tiré hors de
    // ReplaceObjectsCommand.
    QCOMPARE(changes.size(), std::size_t(3));
    QCOMPARE(insertedCount(changes), std::size_t(3));
    QCOMPARE(d.nextId(), nextBefore + 3);
    const std::vector<core::ObjectId> ids = raw->addedIds();
    QCOMPARE(ids.size(), std::size_t(3));

    const auto& group = asGroup(d.page(0).objects.back());
    QCOMPARE(d.page(0).objects.back().common.id, ids[2]);
    QCOMPARE(group.members.size(), std::size_t(2));
    QCOMPARE(group.members[0], ids[0]);
    QCOMPARE(group.members[1], ids[1]);
    for (core::ObjectId member : group.members) {
        QVERIFY(member != a && member != b);
        QVERIFY(hasObject(d, member));
    }
    QCOMPARE(asGroup(objectById(d, g)).members, (std::vector<core::ObjectId>{a, b})); // original intact

    stack.undo(d);
    QCOMPARE(d.page(0).objects.size(), std::size_t(4)); // leurre, a, b, g
    stack.redo(d);                                      // mêmes identifiants, références toujours justes
    QCOMPARE(raw->addedIds(), ids);
    QCOMPARE(d.nextId(), nextBefore + 3);
    const auto& again = asGroup(d.page(0).objects.back());
    QCOMPARE(again.members, (std::vector<core::ObjectId>{ids[0], ids[1]}));
    QVERIFY(hasObject(d, again.members[0]) && hasObject(d, again.members[1]));
}

void TestClipboardEdits::nestedGroupsRemapped()
{
    // g2 = {g1, c}, g1 = {a, b} : l'intérieur avant l'extérieur, et l'extérieur cite la copie de g1.
    core::Document d = blank();
    addRect(d, {500, 500, 10, 10}); // leurre : identifiants d'origine distincts des rangs provisoires
    const auto a = addRect(d, {0, 0, 10, 10});
    const auto b = addRect(d, {20, 0, 10, 10});
    const auto c = addRect(d, {40, 0, 10, 10});
    const auto g1 = addGroup(d, {a, b});
    const auto g2 = addGroup(d, {g1, c});
    const auto prepared = core::preparePaste(core::closureForCopy(d.page(0), {g2}), 0, 0);
    QCOMPARE(prepared.size(), std::size_t(5));
    QVERIFY(std::holds_alternative<core::Group>(core::body(prepared[3])));
    QVERIFY(std::holds_alternative<core::Group>(core::body(prepared[4])));
    QCOMPARE(asGroup(prepared[4]).members.size(), std::size_t(2)); // l'extérieur en dernier
    auto command = std::make_unique<core::PasteObjectsCommand>(0, prepared);
    core::PasteObjectsCommand* raw = command.get();
    core::UndoStack stack;
    stack.push(d, std::move(command));
    const std::vector<core::ObjectId> ids = raw->addedIds();
    QCOMPARE(ids.size(), std::size_t(5));
    const auto& inner = asGroup(objectById(d, ids[3]));
    const auto& outer = asGroup(objectById(d, ids[4]));
    QCOMPARE(inner.members, (std::vector<core::ObjectId>{ids[0], ids[1]}));
    QCOMPARE(outer.members, (std::vector<core::ObjectId>{ids[3], ids[2]})); // ordre des membres gardé
}

void TestClipboardEdits::connectorEndsRemapped()
{
    // Connecteur entre a et b, copié avec a seulement : objectA remappé vers la copie de a, objectB remis à
    // 0.
    core::Document d = blank();
    addRect(d, {500, 500, 10, 10}); // leurre : identifiants d'origine distincts des rangs provisoires
    const auto a = addRect(d, {0, 0, 10, 10});
    const auto b = addRect(d, {100, 0, 10, 10});
    const auto c = addConnector(d, a, b);
    const core::ObjectId nextBefore = d.nextId();
    auto command = std::make_unique<core::PasteObjectsCommand>(
        0, core::preparePaste(core::closureForCopy(d.page(0), {a, c}), 20, 20));
    core::PasteObjectsCommand* raw = command.get();
    core::UndoStack stack;
    const core::Changes changes = stack.push(d, std::move(command));
    QCOMPARE(changes.size(), std::size_t(2));
    QCOMPARE(d.nextId(), nextBefore + 2);
    const std::vector<core::ObjectId> ids = raw->addedIds();
    QCOMPARE(ids.size(), std::size_t(2));
    const auto& link = asConnector(d.page(0).objects.back());
    QVERIFY(link.objectA != a && hasObject(d, link.objectA));
    QCOMPARE(link.objectA, ids[0]);
    QCOMPARE(link.objectB, core::ObjectId(0));
    QCOMPARE(asConnector(objectById(d, c)).objectB, b); // original intact
    stack.undo(d);
    stack.redo(d);
    QCOMPARE(asConnector(d.page(0).objects.back()).objectA, ids[0]);
}

void TestClipboardEdits::pasteOnObjectKeepsItsLayer()
{
    core::Document d = blank();
    const auto a = addRect(d, {0, 0, 10, 10}, core::Layer::Top);
    const auto target = addRect(d, {200, 200, 100, 100}, core::Layer::Bottom);
    const auto copies = core::preparePasteOnto(core::closureForCopy(d.page(0), {a}), objectById(d, target));
    QCOMPARE(copies.size(), std::size_t(1));
    QCOMPARE(copies[0].common.layer, core::Layer::Bottom);
    QCOMPARE(boundsOf(copies[0]).center().x, 250.0);
    QCOMPARE(boundsOf(copies[0]).center().y, 250.0);
}

void TestClipboardEdits::cutIsOneStep()
{
    core::Document d = blank();
    const auto a = addRect(d, {0, 0, 10, 10});
    const auto b = addStroke(d);
    core::UndoStack stack;
    auto cut = core::makeCutCommand(d.page(0), 0, {a, b});
    QVERIFY(cut != nullptr);
    stack.push(d, std::move(cut));
    QVERIFY(d.page(0).objects.empty());
    stack.undo(d);
    QCOMPARE(d.page(0).objects.size(), std::size_t(2));

    // Couper un groupe emporte ses membres.
    core::Document e = blank();
    const auto x = addRect(e, {0, 0, 10, 10});
    const auto y = addRect(e, {20, 0, 10, 10});
    addRect(e, {40, 0, 10, 10});
    const auto g = addGroup(e, {x, y});
    core::UndoStack other;
    auto cutGroup = core::makeCutCommand(e.page(0), 0, {g});
    QVERIFY(cutGroup != nullptr);
    other.push(e, std::move(cutGroup));
    QCOMPARE(e.page(0).objects.size(), std::size_t(1));
    other.undo(e);
    QCOMPARE(e.page(0).objects.size(), std::size_t(4));
}

QTEST_GUILESS_MAIN(TestClipboardEdits)
#include "tst_core_clipboardedits.moc"
