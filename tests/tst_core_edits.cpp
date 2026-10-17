#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/undo.h>

#include <QTest>

#include <limits>
#include <memory>

using namespace actcraft::core;

namespace {

PageObject onLayer(Layer layer, ObjectId id)
{
    Stroke s;
    s.points = {{10, 10, 0.5f}, {30, 10, 0.5f}};
    s.baseWidth = 4;
    PageObject o = s;
    o.common.layer = layer;
    o.common.id = id;
    return o;
}

} // namespace

class TstCoreEdits : public QObject
{
    Q_OBJECT

private slots:
    void newObjectGoesAtTopOfItsLayer()
    {
        Page p;
        insertAtTopOfLayer(p, onLayer(Layer::Middle, 1));
        insertAtTopOfLayer(p, onLayer(Layer::Top, 2));
        QCOMPARE(insertAtTopOfLayer(p, onLayer(Layer::Middle, 3)), std::size_t{1});
        QCOMPARE(insertAtTopOfLayer(p, onLayer(Layer::Bottom, 4)), std::size_t{0});
        QCOMPARE(insertAtTopOfLayer(p, onLayer(Layer::Background, 5)), std::size_t{0});
        QCOMPARE(p.objects[0].common.id, ObjectId{5});
        QCOMPARE(p.objects[1].common.id, ObjectId{4});
        QCOMPARE(p.objects[2].common.id, ObjectId{1});
        QCOMPARE(p.objects[3].common.id, ObjectId{3});
        QCOMPARE(p.objects[4].common.id, ObjectId{2});
        QVERIFY(isLayerOrdered(p));
    }

    void depthCountsWithinTheLayer()
    {
        Page p;
        insertAtTopOfLayer(p, onLayer(Layer::Bottom, 1));
        insertAtTopOfLayer(p, onLayer(Layer::Middle, 2));
        insertAtTopOfLayer(p, onLayer(Layer::Middle, 3));
        QCOMPARE(depthInLayer(p, 1), std::size_t{0});
        QCOMPARE(depthInLayer(p, 2), std::size_t{0});
        QCOMPARE(depthInLayer(p, 3), std::size_t{1});
        QCOMPARE(indexOf(p, 3), std::size_t{2});
        QCOMPARE(indexOf(p, 99), p.objects.size());
    }

    void layerOrderIsChecked()
    {
        Page p;
        p.objects = {onLayer(Layer::Top, 1), onLayer(Layer::Middle, 2)};
        QVERIFY(!isLayerOrdered(p));
    }

    void boundsOnPageFollowsTheTransformAndTheWidth()
    {
        PageObject o = onLayer(Layer::Top, 1); // points (10,10)-(30,10), largeur 4 à pression 0,5
        RectF r = boundsOnPage(o);
        QCOMPARE(r.x, 8.0);
        QCOMPARE(r.y, 8.0);
        QCOMPARE(r.width, 24.0);
        QCOMPARE(r.height, 4.0);
        o.common.transform = translation(100, 0);
        r = boundsOnPage(o);
        QCOMPARE(r.x, 108.0);
        Text t;
        t.bounds = {0, 0, 50, 20};
        PageObject text = t;
        text.common.transform = rotation(90);
        r = boundsOnPage(text);
        QCOMPARE(r.x, -20.0);
        QCOMPARE(r.y, 0.0);
        QCOMPARE(r.width, 20.0);
        QCOMPARE(r.height, 50.0);
    }

    void addObjectAllocatesAnIdOnceAndUndoes()
    {
        Document d;
        UndoStack stack;
        Stroke s;
        s.points = {{1, 1, 0.5f}};
        PageObject o = s;
        o.common.layer = Layer::Top;
        auto command = std::make_unique<AddObjectCommand>(0, o);
        AddObjectCommand* raw = command.get();
        Changes changes = stack.push(d, std::move(command));
        const ObjectId id = raw->id();
        QVERIFY(id != 0);
        QCOMPARE(d.page(0).objects.size(), std::size_t{1});
        QCOMPARE(d.page(0).objects[0].common.id, id);
        QCOMPARE(changes.size(), std::size_t{1});
        QCOMPARE(changes[0].kind, Change::Kind::ObjectInserted);
        QCOMPARE(changes[0].page, std::size_t{0});
        QCOMPARE(changes[0].object, id);
        changes = stack.undo(d);
        QVERIFY(d.page(0).objects.empty());
        QCOMPARE(changes[0].kind, Change::Kind::ObjectRemoved);
        QCOMPARE(changes[0].object, id);
        stack.redo(d);
        QCOMPARE(d.page(0).objects[0].common.id, id); // même identifiant au rétablissement
        QCOMPARE(d.nextId(), id + 1);
    }

    void addObjectLandsAtTopOfItsLayerNotAtTheEnd()
    {
        Document d;
        UndoStack stack;
        stack.push(d, std::make_unique<AddObjectCommand>(0, onLayer(Layer::Top, 0)));
        stack.push(d, std::make_unique<AddObjectCommand>(0, onLayer(Layer::Middle, 0)));
        QCOMPARE(d.page(0).objects[0].common.layer, Layer::Middle);
        QCOMPARE(d.page(0).objects[1].common.layer, Layer::Top);
    }

    void addObjectInsertsNothingWhenIdsAreExhausted()
    {
        Document d;
        d.reserveIdsBelow(std::numeric_limits<ObjectId>::max()); // espace épuisé : allocateId() rend 0
        QCOMPARE(d.nextId(), std::numeric_limits<ObjectId>::max());
        UndoStack stack;
        auto command = std::make_unique<AddObjectCommand>(0, onLayer(Layer::Top, 0));
        AddObjectCommand* raw = command.get();
        const Changes changes = stack.push(d, std::move(command));
        QVERIFY(changes.empty());
        QVERIFY(d.page(0).objects.empty()); // jamais d'objet d'identifiant 0
        QCOMPARE(raw->id(), ObjectId{0});
        QVERIFY(stack.undo(d).empty()); // annuler et rétablir ne font rien non plus
        QVERIFY(stack.redo(d).empty());
        QVERIFY(d.page(0).objects.empty());
    }

    void addPageInsertsABlankPageOfTheGivenSize()
    {
        Document d;
        UndoStack stack;
        Changes changes = stack.push(d, std::make_unique<AddPageCommand>(1, 800, 600));
        QCOMPARE(d.pageCount(), std::size_t{2});
        QCOMPARE(d.page(1).width, 800.0);
        QCOMPARE(changes[0].kind, Change::Kind::PageInserted);
        QCOMPARE(changes[0].page, std::size_t{1});
        changes = stack.undo(d);
        QCOMPARE(d.pageCount(), std::size_t{1});
        QCOMPARE(changes[0].kind, Change::Kind::PageRemoved);
    }
};

QTEST_GUILESS_MAIN(TstCoreEdits)
#include "tst_core_edits.moc"
