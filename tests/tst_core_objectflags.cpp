#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/objectflags.h>
#include <actcraft/core/undo.h>

#include <QTest>

#include <algorithm>
#include <memory>
#include <vector>

namespace core = actcraft::core;

namespace {

core::Document blank()
{
    return core::Document{};
}

core::ObjectId addRect(core::Document& d, core::RectF bounds)
{
    core::Shape s;
    s.bounds = bounds;
    core::PageObject o = s;
    o.common.id = d.nextId();
    core::insertAtTopOfLayer(d.page(0), std::move(o));
    d.refreshNextId();
    return d.page(0).objects.back().common.id;
}

core::PageObject& objectById(core::Document& d, core::ObjectId id)
{
    auto& page = d.page(0);
    return page.objects[core::indexOf(page, id)];
}

int countKind(const core::Changes& changes, core::Change::Kind kind)
{
    return static_cast<int>(
        std::count_if(changes.begin(), changes.end(), [kind](const auto& c) { return c.kind == kind; }));
}

} // namespace

class TestFlags : public QObject
{
    Q_OBJECT
private slots:
    void toggleLockIsOneStep()
    {
        auto d = blank();
        const auto a = addRect(d, {0, 0, 10, 10});
        const auto b = addRect(d, {20, 0, 10, 10});
        core::UndoStack stack;
        stack.push(d, std::make_unique<core::SetObjectFlagCommand>(0, std::vector{a, b},
                                                                   core::ObjectFlag::Locked, true));
        QVERIFY(core::flagOfAll(d.page(0), {a, b}, core::ObjectFlag::Locked));
        QCOMPARE(stack.count(), std::size_t{1});
        stack.undo(d);
        QVERIFY(!objectById(d, a).common.locked && !objectById(d, b).common.locked);
    }
    void mixedSelectionLocksAll()
    {
        auto d = blank();
        const auto a = addRect(d, {0, 0, 10, 10});
        const auto b = addRect(d, {20, 0, 10, 10});
        objectById(d, a).common.locked = true;
        QVERIFY(!core::flagOfAll(d.page(0), {a, b}, core::ObjectFlag::Locked));
        // La valeur posée est « vrai » pour tous ; l'annulation rend à chacun son état d'avant.
        core::UndoStack stack;
        stack.push(d, std::make_unique<core::SetObjectFlagCommand>(0, std::vector{a, b},
                                                                   core::ObjectFlag::Locked, true));
        QVERIFY(core::flagOfAll(d.page(0), {a, b}, core::ObjectFlag::Locked));
        stack.undo(d);
        QVERIFY(objectById(d, a).common.locked);
        QVERIFY(!objectById(d, b).common.locked);
    }
    void hiddenObjectReportsAChange()
    {
        auto d = blank();
        const auto a = addRect(d, {0, 0, 10, 10});
        core::Changes changes;
        core::SetObjectFlagCommand command(0, {a}, core::ObjectFlag::Hidden, true);
        command.redo(d, changes);
        QVERIFY(!objectById(d, a).common.visible);
        QCOMPARE(countKind(changes, core::Change::Kind::ObjectChanged), 1);
        core::Changes back;
        command.undo(d, back);
        QVERIFY(objectById(d, a).common.visible);
        QCOMPARE(countKind(back, core::Change::Kind::ObjectChanged), 1);
        core::Changes none;
        core::SetObjectFlagCommand lock(0, {a}, core::ObjectFlag::Locked, true);
        lock.redo(d, none);
        QVERIFY(none.empty());
    }
    void dragCopyFlagRoundTrips()
    {
        auto d = blank();
        const auto a = addRect(d, {0, 0, 10, 10});
        core::UndoStack stack;
        stack.push(d, std::make_unique<core::SetObjectFlagCommand>(0, std::vector{a},
                                                                   core::ObjectFlag::DragCopy, true));
        QVERIFY(objectById(d, a).common.dragCopy);
        stack.undo(d);
        QVERIFY(!objectById(d, a).common.dragCopy);
    }
};

QTEST_GUILESS_MAIN(TestFlags)
#include "tst_core_objectflags.moc"
