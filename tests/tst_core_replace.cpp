#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/pagecache.h>
#include <actcraft/core/replace.h>
#include <actcraft/core/undo.h>

#include <QStringList>
#include <QTest>

#include <memory>
#include <utility>
#include <vector>

using namespace actcraft::core;

namespace {

// Cache espion : l'ordre exact des appels que applyToCache lui envoie.
struct SpyCache final : actcraft::core::PageCache
{
    QStringList calls;
    void insert(ObjectId id) override { calls << QStringLiteral("insert %1").arg(id); }
    void remove(ObjectId id, const RectF&) override { calls << QStringLiteral("remove %1").arg(id); }
    void invalidate(ObjectId id, const CacheArea&) override
    {
        calls << QStringLiteral("invalidate %1").arg(id);
    }
};

PageObject strokeAt(double x, Layer layer = Layer::Top)
{
    Stroke s;
    s.points = {{x, 10, 0.5f}, {x + 20, 30, 0.5f}};
    PageObject o = s;
    o.common.layer = layer;
    return o;
}

PageObject shapeOn(Layer layer)
{
    Shape s;
    s.bounds = {300, 300, 40, 40};
    PageObject o = s;
    o.common.layer = layer;
    return o;
}

ObjectId add(Document& document, UndoStack& stack, PageObject object)
{
    auto command = std::make_unique<AddObjectCommand>(0, std::move(object));
    const AddObjectCommand* raw = command.get();
    stack.push(document, std::move(command));
    return raw->id();
}

std::vector<ObjectId> idsOf(const Page& page)
{
    std::vector<ObjectId> ids;
    for (const PageObject& o : page.objects) {
        ids.push_back(o.common.id);
    }
    return ids;
}

QString describe(const std::vector<ObjectId>& ids)
{
    QStringList parts;
    for (const ObjectId id : ids) {
        parts << QString::number(id);
    }
    return parts.join(u", ");
}

bool sameRect(const RectF& a, const RectF& b)
{
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

QString call(const char* kind, ObjectId id)
{
    return QStringLiteral("%1 %2").arg(QLatin1StringView(kind)).arg(id);
}

} // namespace

// Commande de remplacement unique (socle des lots 3 et 7) : placement r - k, contrat de Change (retraits puis
// ajouts, inverse exact), réglages de présentation sans redessin du cache (I3).
class TstCoreReplace : public QObject
{
    Q_OBJECT

private:
    Document m_document;
    UndoStack m_stack;
    ObjectId m_a = 0;
    ObjectId m_b = 0;
    ObjectId m_c = 0;

    Page& page() { return m_document.page(0); }

private slots:
    // Page de trois traits a, b, c de la couche supérieure, ajoutés dans cet ordre.
    void init()
    {
        m_document = Document();
        m_stack.clear();
        m_a = add(m_document, m_stack, strokeAt(10));
        m_b = add(m_document, m_stack, strokeAt(50));
        m_c = add(m_document, m_stack, strokeAt(90));
        QCOMPARE(idsOf(page()), (std::vector<ObjectId>{m_a, m_b, m_c}));
    }

    void oneGroupMakesOneStep()
    {
        const std::size_t before = m_stack.index();
        auto command = std::make_unique<ReplaceObjectsCommand>(0, std::vector<ObjectId>{m_b},
                                                               std::vector<PageObject>{shapeOn(Layer::Top)});
        const ReplaceObjectsCommand* raw = command.get();
        m_stack.push(m_document, std::move(command));
        QCOMPARE(m_stack.index(), before + 1);
        QCOMPARE(page().objects.size(), std::size_t{3});
        QVERIFY(std::get_if<Shape>(&page().objects[1]) != nullptr);
        QCOMPARE(raw->addedIds().size(), std::size_t{1});
        QCOMPARE(page().objects[1].common.id, raw->addedIds()[0]);
        m_stack.undo(m_document);
        QCOMPARE(idsOf(page()), (std::vector<ObjectId>{m_a, m_b, m_c}));
        QVERIFY(std::get_if<Stroke>(&page().objects[1]) != nullptr);
    }

    void piecesTakeTheOriginalRank()
    {
        auto command = std::make_unique<ReplaceObjectsCommand>(
            0, std::vector<ObjectId>{m_b}, std::vector<PageObject>{strokeAt(50), strokeAt(60)});
        const ReplaceObjectsCommand* raw = command.get();
        m_stack.push(m_document, std::move(command));
        const std::vector<ObjectId> pieces = raw->addedIds();
        QCOMPARE(pieces.size(), std::size_t{2});
        const std::vector<ObjectId> expected{m_a, pieces[0], pieces[1], m_c};
        QVERIFY2(idsOf(page()) == expected, qPrintable(describe(idsOf(page()))));
    }

    // a, s1, b, s2, c avec {s1, s2} → {f} : f prend le rang r - k (r = 3, rang de s2 ; k = 1, s1 sous lui).
    // Sommet de couche : a, b, c, f ; rang du plus bas retiré : a, f, b, c ; rang r sans k : a, b, c, f.
    void groupOfTwoTakesTheHigherRank()
    {
        m_document = Document();
        m_stack.clear();
        const ObjectId a = add(m_document, m_stack, strokeAt(10));
        const ObjectId s1 = add(m_document, m_stack, strokeAt(30));
        const ObjectId b = add(m_document, m_stack, strokeAt(50));
        const ObjectId s2 = add(m_document, m_stack, strokeAt(70));
        const ObjectId c = add(m_document, m_stack, strokeAt(90));
        auto command = std::make_unique<ReplaceObjectsCommand>(0, std::vector<ObjectId>{s1, s2},
                                                               std::vector<PageObject>{shapeOn(Layer::Top)});
        const ReplaceObjectsCommand* raw = command.get();
        m_stack.push(m_document, std::move(command));
        const std::vector<ObjectId> expected{a, b, raw->addedIds()[0], c};
        QVERIFY2(idsOf(page()) == expected, qPrintable(describe(idsOf(page()))));
        m_stack.undo(m_document);
        const std::vector<ObjectId> original{a, s1, b, s2, c};
        QVERIFY2(idsOf(page()) == original, qPrintable(describe(idsOf(page()))));
    }

    void otherLayerGoesToTopOfLayer()
    {
        const ObjectId middle = add(m_document, m_stack, strokeAt(5, Layer::Middle));
        QCOMPARE(idsOf(page()), (std::vector<ObjectId>{middle, m_a, m_b, m_c}));
        auto command = std::make_unique<ReplaceObjectsCommand>(
            0, std::vector<ObjectId>{m_b}, std::vector<PageObject>{shapeOn(Layer::Middle)});
        const ReplaceObjectsCommand* raw = command.get();
        m_stack.push(m_document, std::move(command));
        const std::vector<ObjectId> expected{middle, raw->addedIds()[0], m_a, m_c};
        QVERIFY2(idsOf(page()) == expected, qPrintable(describe(idsOf(page()))));
        QVERIFY(isLayerOrdered(page()));
    }

    void absentIdIgnored()
    {
        Changes changes;
        ReplaceObjectsCommand withAbsent(0, std::vector<ObjectId>{999, m_b},
                                         std::vector<PageObject>{strokeAt(50)});
        withAbsent.redo(m_document, changes);
        QCOMPARE(changes.size(), std::size_t{2});
        QCOMPARE(changes[0].kind, Change::Kind::ObjectRemoved);
        QCOMPARE(changes[0].object, m_b);
        const std::vector<ObjectId> expected{m_a, withAbsent.addedIds()[0], m_c};
        QVERIFY2(idsOf(page()) == expected, qPrintable(describe(idsOf(page()))));

        const std::uint64_t before = fingerprint(page());
        Changes none;
        ReplaceObjectsCommand nothing(0, std::vector<ObjectId>{999}, std::vector<PageObject>{});
        nothing.redo(m_document, none);
        QVERIFY(none.empty());
        QCOMPARE(fingerprint(page()), before);
        nothing.undo(m_document, none);
        QVERIFY(none.empty());
        QCOMPARE(fingerprint(page()), before);
    }

    void addedIdsStableAcrossRedo()
    {
        PageObject carried = strokeAt(50);
        carried.common.id = 12345; // ignoré : l'identifiant est tiré au premier redo
        auto command = std::make_unique<ReplaceObjectsCommand>(
            0, std::vector<ObjectId>{m_b}, std::vector<PageObject>{carried, strokeAt(60)});
        const ReplaceObjectsCommand* raw = command.get();
        QVERIFY(raw->addedIds().empty());
        m_stack.push(m_document, std::move(command));
        const std::vector<ObjectId> first = raw->addedIds();
        QCOMPARE(first.size(), std::size_t{2});
        QVERIFY(first[0] != 0 && first[1] != 0 && first[0] != first[1]);
        QVERIFY(first[0] != 12345 && first[0] != m_a && first[0] != m_b && first[0] != m_c);
        const std::uint64_t afterFirst = fingerprint(page());
        m_stack.undo(m_document);
        QCOMPARE(idsOf(page()), (std::vector<ObjectId>{m_a, m_b, m_c}));
        m_stack.redo(m_document);
        QCOMPARE(raw->addedIds(), first);
        QCOMPARE(fingerprint(page()), afterFirst);
        QVERIFY(hasUniqueIds(m_document));
    }

    // Contrat de Change au redo : pour chaque groupe, ses retraits puis ses ajouts.
    void cacheSeesRemoveThenInsert()
    {
        const PageObject objectA = page().objects[0];
        const PageObject objectC = page().objects[2];
        auto command = std::make_unique<ReplaceObjectsCommand>(
            0, std::vector<ObjectReplacement>{
                   ObjectReplacement{{m_a}, {strokeAt(10)}},
                   ObjectReplacement{{m_c}, {strokeAt(90), strokeAt(100)}},
               });
        const ReplaceObjectsCommand* raw = command.get();
        const Changes changes = m_stack.push(m_document, std::move(command));
        const std::vector<ObjectId>& added = raw->addedIds();
        QCOMPARE(added.size(), std::size_t{3});
        SpyCache spy;
        QCOMPARE(applyToCache(changes, 0, spy), std::optional<std::size_t>{0});
        const QStringList expected{call("remove", m_a), call("insert", added[0]), call("remove", m_c),
                                   call("insert", added[1]), call("insert", added[2])};
        QCOMPARE(spy.calls, expected);
        QVERIFY(sameRect(changes[0].area, boundsOnPage(objectA)));
        QVERIFY(sameRect(changes[2].area, boundsOnPage(objectC)));
        QVERIFY(sameRect(changes[1].area, boundsOnPage(page().objects[indexOf(page(), added[0])])));
    }

    // À l'annulation, l'inverse exact : groupes à rebours, ajouts retirés à rebours, retirés remis.
    void undoIsTheExactInverse()
    {
        const std::uint64_t original = fingerprint(page());
        auto command = std::make_unique<ReplaceObjectsCommand>(
            0, std::vector<ObjectReplacement>{
                   ObjectReplacement{{m_a}, {strokeAt(10)}},
                   ObjectReplacement{{m_c}, {strokeAt(90), strokeAt(100)}},
               });
        const ReplaceObjectsCommand* raw = command.get();
        m_stack.push(m_document, std::move(command));
        const std::vector<ObjectId> added = raw->addedIds();
        std::vector<RectF> addedAreas;
        for (const ObjectId id : added) {
            addedAreas.push_back(boundsOnPage(page().objects[indexOf(page(), id)]));
        }
        const Changes changes = m_stack.undo(m_document);
        SpyCache spy;
        applyToCache(changes, 0, spy);
        const QStringList expected{call("remove", added[2]), call("remove", added[1]), call("insert", m_c),
                                   call("remove", added[0]), call("insert", m_a)};
        QCOMPARE(spy.calls, expected);
        QVERIFY(sameRect(changes[0].area, addedAreas[2]));
        QVERIFY(sameRect(changes[1].area, addedAreas[1]));
        QVERIFY(sameRect(changes[3].area, addedAreas[0]));
        QCOMPARE(idsOf(page()), (std::vector<ObjectId>{m_a, m_b, m_c}));
        QCOMPARE(fingerprint(page()), original);
    }

    // I3 : un réglage de présentation ne touche aucun pixel du cache (ni PageChanged, ni ObjectChanged).
    void presentationTouchesNoCache()
    {
        PagePresentation p;
        p.curtain = CurtainMode::Top;
        const Changes changes = m_stack.push(m_document, std::make_unique<SetPagePresentationCommand>(0, p));
        QCOMPARE(changes.size(), std::size_t{1});
        QCOMPARE(changes[0].kind, Change::Kind::PageSettingsChanged);
        QCOMPARE(changes[0].page, std::size_t{0});
        SpyCache spy;
        QCOMPARE(applyToCache(changes, 0, spy), std::optional<std::size_t>{0});
        QVERIFY2(spy.calls.isEmpty(), qPrintable(spy.calls.join(u"; ")));
        const Changes undone = m_stack.undo(m_document);
        QCOMPARE(undone.size(), std::size_t{1});
        QCOMPARE(undone[0].kind, Change::Kind::PageSettingsChanged);
        applyToCache(undone, 0, spy);
        QVERIFY2(spy.calls.isEmpty(), qPrintable(spy.calls.join(u"; ")));
    }

    void setPagePresentationMakesOneStep()
    {
        PagePresentation p;
        p.spotlight = SpotlightShape::Circle;
        p.transition = "fade";
        const std::size_t before = m_stack.index();
        m_stack.push(m_document, std::make_unique<SetPagePresentationCommand>(0, p));
        QCOMPARE(m_stack.index(), before + 1);
        QCOMPARE(page().presentation.spotlight, SpotlightShape::Circle);
        QCOMPARE(page().presentation.transition, std::string("fade"));
        Changes same;
        SetPagePresentationCommand again(0, p);
        again.redo(m_document, same);
        QVERIFY(same.empty()); // déjà la valeur de la page : rien ne change
        m_stack.undo(m_document);
        QVERIFY(isDefault(page().presentation));
    }
};

QTEST_GUILESS_MAIN(TstCoreReplace)
#include "tst_core_replace.moc"
