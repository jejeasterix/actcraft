#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/lift.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/core/pagecache.h>
#include <actcraft/core/replace.h>

#include <QStringList>
#include <QTest>

#include <memory>
#include <set>
#include <utility>
#include <vector>

using namespace actcraft::core;

namespace {

Document twoStrokes(ObjectId& a, ObjectId& b)
{
    Document doc;
    Stroke s;
    s.points = {{100, 100, 0.5f}, {300, 100, 0.5f}};
    Changes ignored;
    auto annotation = [](Stroke st) {
        PageObject o{std::move(st)};
        o.common.layer = Layer::Top; // annotation (F-230)
        return o;
    };
    AddObjectCommand first(0, annotation(s));
    first.redo(doc, ignored);
    a = first.id();
    s.points = {{100, 200, 0.5f}, {300, 200, 0.5f}};
    AddObjectCommand second(0, annotation(s));
    second.redo(doc, ignored);
    b = second.id();
    return doc;
}

int count(const Changes& changes, Change::Kind kind, ObjectId id)
{
    int n = 0;
    for (const Change& c : changes) {
        n += (c.kind == kind && c.object == id) ? 1 : 0;
    }
    return n;
}

std::set<ObjectId> idsOf(const Page& page)
{
    std::set<ObjectId> ids;
    for (const PageObject& o : page.objects) {
        ids.insert(o.common.id);
    }
    return ids;
}

// Cache qui tient la liste des objets qu'il dessine et relève toute demande incohérente avec elle : retrait
// ou invalidation d'un objet qu'il n'a pas, insertion d'un objet qu'il a déjà (invariant de L3-T4).
struct StrictCache final : PageCache
{
    std::set<ObjectId> held;
    QStringList faults;
    void insert(ObjectId id) override
    {
        if (!held.insert(id).second) {
            faults << QStringLiteral("insert %1: already held").arg(id);
        }
    }
    void remove(ObjectId id, const RectF&) override
    {
        if (held.erase(id) == 0) {
            faults << QStringLiteral("remove %1: not held").arg(id);
        }
    }
    void invalidate(ObjectId id, const CacheArea&) override
    {
        if (id != 0 && held.count(id) == 0) {
            faults << QStringLiteral("invalidate %1: not held").arg(id);
        }
    }
};

enum class Drop
{
    Move,
    MoveOther,
    Erase,
    RemoveBoth,
    Translucency,
    Cancel,
};

} // namespace

class TstCoreLift : public QObject
{
    Q_OBJECT

private slots:
    void takeAndRestoreKeepRanks()
    {
        ObjectId a = 0, b = 0;
        Document doc = twoStrokes(a, b);
        const std::uint64_t before = fingerprint(doc.page(0));
        auto lifted = takeObjects(doc.page(0), {a});
        QCOMPARE(lifted.size(), std::size_t{1});
        QCOMPARE(lifted[0].index, std::size_t{0});
        QCOMPARE(doc.page(0).objects.size(), std::size_t{1});
        const Changes up = liftChanges(0, lifted);
        QCOMPARE(count(up, Change::Kind::ObjectRemoved, a), 1);
        restoreObjects(doc.page(0), lifted);
        QCOMPARE(fingerprint(doc.page(0)), before);
    }

    // Rangs non contigus, identifiants donnés dans le désordre et inconnus : la liste suit la page, la remise
    // rend la page d'avant.
    void takeAndRestoreInterleavedRanks()
    {
        ObjectId a = 0, b = 0;
        Document doc = twoStrokes(a, b);
        Stroke s;
        s.points = {{100, 300, 0.5f}, {300, 300, 0.5f}};
        PageObject third{s};
        third.common.layer = Layer::Top;
        Changes ignored;
        AddObjectCommand add(0, third);
        add.redo(doc, ignored);
        const ObjectId c = add.id();
        const std::uint64_t before = fingerprint(doc.page(0));

        auto lifted = takeObjects(doc.page(0), {c, 987654, a, c});
        QCOMPARE(lifted.size(), std::size_t{2});
        QCOMPARE(lifted[0].object.common.id, a);
        QCOMPARE(lifted[0].index, std::size_t{0});
        QCOMPARE(lifted[1].object.common.id, c);
        QCOMPARE(lifted[1].index, std::size_t{2});
        QCOMPARE(doc.page(0).objects.size(), std::size_t{1});
        QCOMPARE(doc.page(0).objects[0].common.id, b);
        const Changes up = liftChanges(0, lifted);
        QCOMPARE(up.size(), std::size_t{2});
        QCOMPARE(up[0].area.x, boundsOnPage(lifted[0].object).x);
        restoreObjects(doc.page(0), lifted);
        QCOMPARE(fingerprint(doc.page(0)), before);
    }

    void liftThenDropMoveYieldsInsertOnly()
    {
        ObjectId a = 0, b = 0;
        Document doc = twoStrokes(a, b);
        auto lifted = takeObjects(doc.page(0), {a});
        restoreObjects(doc.page(0), lifted); // dépôt : originaux remis, puis la commande
        TransformObjectsCommand move(0, {a}, translation(50, 0));
        Changes command;
        move.redo(doc, command);
        QCOMPARE(count(command, Change::Kind::ObjectChanged, a), 1);
        const Changes cache = changesAfterLift(0, lifted, command, doc.page(0));
        QCOMPARE(count(cache, Change::Kind::ObjectInserted, a), 1);
        QCOMPARE(count(cache, Change::Kind::ObjectChanged, a), 0);
        QCOMPARE(count(cache, Change::Kind::ObjectRemoved, a), 0);
        QCOMPARE(cache.size(), std::size_t{1});
    }

    void liftThenCancelReinserts()
    {
        ObjectId a = 0, b = 0;
        Document doc = twoStrokes(a, b);
        auto lifted = takeObjects(doc.page(0), {a, b});
        restoreObjects(doc.page(0), lifted);
        const Changes cache = changesAfterLift(0, lifted, {}, doc.page(0));
        QCOMPARE(count(cache, Change::Kind::ObjectInserted, a), 1);
        QCOMPARE(count(cache, Change::Kind::ObjectInserted, b), 1);
        QCOMPARE(cache.size(), std::size_t{2});
    }

    void erasePiecesInsertedNoRemoveOfLifted()
    {
        ObjectId a = 0, b = 0;
        Document doc = twoStrokes(a, b);
        auto lifted = takeObjects(doc.page(0), {a});
        restoreObjects(doc.page(0), lifted);
        Stroke leftStroke;
        leftStroke.points = {{100, 100, 0.5f}, {180, 100, 0.5f}};
        Stroke rightStroke;
        rightStroke.points = {{220, 100, 0.5f}, {300, 100, 0.5f}};
        PageObject left{leftStroke};
        left.common.layer = Layer::Top;
        PageObject right{rightStroke};
        right.common.layer = Layer::Top;
        ReplaceObjectsCommand erase(0, std::vector<ObjectId>{a}, std::vector<PageObject>{left, right});
        Changes command;
        erase.redo(doc, command);
        const Changes cache = changesAfterLift(0, lifted, command, doc.page(0));
        QCOMPARE(count(cache, Change::Kind::ObjectRemoved, a), 0);
        QCOMPARE(count(cache, Change::Kind::ObjectInserted, erase.addedIds()[0]), 1);
        QCOMPARE(count(cache, Change::Kind::ObjectInserted, erase.addedIds()[1]), 1);
        QCOMPARE(cache.size(), std::size_t{2});
    }

    void otherObjectsChangesPassThrough()
    {
        ObjectId a = 0, b = 0;
        Document doc = twoStrokes(a, b);
        auto lifted = takeObjects(doc.page(0), {a});
        restoreObjects(doc.page(0), lifted);
        RemoveObjectsCommand both(0, {a, b});
        Changes command;
        both.redo(doc, command);
        const Changes cache = changesAfterLift(0, lifted, command, doc.page(0));
        QCOMPARE(count(cache, Change::Kind::ObjectRemoved, a), 0);  // le cache ne l'a plus
        QCOMPARE(count(cache, Change::Kind::ObjectRemoved, b), 1);  // b n'était pas soulevé
        QCOMPARE(count(cache, Change::Kind::ObjectInserted, a), 0); // a n'est plus dans la page
    }

    // L'invariant lui-même, par le chemin de référence (applyToCache, comme le contrôleur) : un cache qui
    // tient ses objets ne reçoit jamais de retrait ni d'invalidation d'un objet qu'il n'a plus, et finit avec
    // exactement les objets de la page. Rejouer au dépôt les Changes de la commande tels quels le casse.
    void cacheNeverHearsOfAnObjectItNoLongerHas_data()
    {
        QTest::addColumn<int>("dropRow");
        QTest::newRow("move") << static_cast<int>(Drop::Move);
        QTest::newRow("move-other") << static_cast<int>(Drop::MoveOther);
        QTest::newRow("erase") << static_cast<int>(Drop::Erase);
        QTest::newRow("remove-both") << static_cast<int>(Drop::RemoveBoth);
        QTest::newRow("translucency") << static_cast<int>(Drop::Translucency);
        QTest::newRow("cancel") << static_cast<int>(Drop::Cancel);
    }
    void cacheNeverHearsOfAnObjectItNoLongerHas()
    {
        QFETCH(int, dropRow);
        const auto drop = static_cast<Drop>(dropRow);
        ObjectId a = 0, b = 0;
        Document doc = twoStrokes(a, b);
        StrictCache cache;
        cache.held = idsOf(doc.page(0));

        auto lifted = takeObjects(doc.page(0), {a});
        QCOMPARE(applyToCache(liftChanges(0, lifted), 0, cache), std::optional<std::size_t>{0});
        QVERIFY(cache.held == std::set<ObjectId>{b});

        restoreObjects(doc.page(0), lifted);
        std::unique_ptr<UndoCommand> command;
        switch (drop) {
        case Drop::Move:
            command =
                std::make_unique<TransformObjectsCommand>(0, std::vector<ObjectId>{a}, translation(40, 0));
            break;
        case Drop::MoveOther:
            command =
                std::make_unique<TransformObjectsCommand>(0, std::vector<ObjectId>{b}, translation(0, 40));
            break;
        case Drop::Erase: {
            Stroke piece;
            piece.points = {{100, 100, 0.5f}, {180, 100, 0.5f}};
            PageObject left{piece};
            left.common.layer = Layer::Top;
            command = std::make_unique<ReplaceObjectsCommand>(0, std::vector<ObjectId>{a},
                                                              std::vector<PageObject>{left});
            break;
        }
        case Drop::RemoveBoth:
            command = std::make_unique<RemoveObjectsCommand>(0, std::vector<ObjectId>{a, b});
            break;
        case Drop::Translucency:
            command = std::make_unique<SetTranslucencyCommand>(0, std::vector<ObjectId>{a}, std::uint8_t{90});
            break;
        case Drop::Cancel:
            break;
        }
        Changes changes;
        if (command) {
            command->redo(doc, changes);
            QVERIFY(!changes.empty());
        }
        applyToCache(changesAfterLift(0, lifted, changes, doc.page(0)), 0, cache);
        QVERIFY2(cache.faults.isEmpty(), qPrintable(cache.faults.join(QStringLiteral("; "))));
        QVERIFY(cache.held == idsOf(doc.page(0)));
    }
};

QTEST_GUILESS_MAIN(TstCoreLift)
#include "tst_core_lift.moc"
