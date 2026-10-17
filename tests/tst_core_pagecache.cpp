#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/pagecache.h>

#include <QStringList>
#include <QTest>

using namespace actcraft::core;

namespace {

QString describe(const RectF& r)
{
    return QStringLiteral("%1 %2 %3 %4").arg(r.x).arg(r.y).arg(r.width).arg(r.height);
}

class RecordingCache final : public PageCache
{
public:
    QStringList calls;
    void insert(ObjectId id) override { calls << QStringLiteral("insert %1").arg(id); }
    void remove(ObjectId id, const RectF& area) override
    {
        calls << QStringLiteral("remove %1 %2").arg(id).arg(describe(area));
    }
    void invalidate(ObjectId id, const CacheArea& area) override
    {
        calls << QStringLiteral("invalidate %1 %2")
                     .arg(id)
                     .arg(area ? describe(*area) : QStringLiteral("page"));
    }
};

} // namespace

// DA11 : les changements de la pile n'atteignent le cache que par son interface, pour la seule page affichée,
// avec l'identifiant de l'objet et sa zone.
class TstCorePageCache : public QObject
{
    Q_OBJECT

private slots:
    void onlyTheShownPageReachesTheCache()
    {
        const Changes changes{
            Change{Change::Kind::ObjectInserted, 1, 7, RectF{0, 0, 10, 10}},
            Change{Change::Kind::ObjectInserted, 0, 8, RectF{}},
            Change{Change::Kind::ObjectRemoved, 1, 9, RectF{1, 2, 3, 4}},
            Change{Change::Kind::ObjectChanged, 1, 10, RectF{5, 5, 20, 30}},
            Change{Change::Kind::PageChanged, 1, 0, RectF{}},
        };
        RecordingCache cache;
        QCOMPARE(applyToCache(changes, 1, cache), std::optional<std::size_t>{1});
        QCOMPARE(cache.calls, (QStringList{QStringLiteral("insert 7"), QStringLiteral("remove 9 1 2 3 4"),
                                           QStringLiteral("invalidate 10 5 5 20 30"),
                                           QStringLiteral("invalidate 0 page")}));
    }

    // Annuler un ajout : l'objet n'est plus dans le document, son identifiant et son emprise arrivent au
    // cache.
    void removalByUndoCarriesIdAndArea()
    {
        Document d;
        Stroke stroke;
        stroke.baseWidth = 4;
        stroke.points = {{10, 20, 0.5f}, {110, 70, 0.5f}};
        AddObjectCommand add(0, stroke);
        Changes changes;
        add.redo(d, changes);
        const RectF bounds = boundsOnPage(d.page(0).objects.front());
        QVERIFY(bounds.width > 0 && bounds.height > 0);
        changes.clear();
        add.undo(d, changes);
        RecordingCache cache;
        QCOMPARE(applyToCache(changes, 0, cache), std::optional<std::size_t>{0});
        QCOMPARE(cache.calls,
                 QStringList{QStringLiteral("remove %1 %2").arg(add.id()).arg(describe(bounds))});
    }

    // Une zone vide d'objet modifié vaut « toute la page » (contrat de Change) ; le cache, lui, distingue la
    // page entière (std::nullopt) d'une zone, et un retrait sans emprise reste une zone vide.
    void wholePageIsDistinctFromAnEmptyArea()
    {
        const Changes changes{
            Change{Change::Kind::ObjectChanged, 0, 11, RectF{}},
            Change{Change::Kind::ObjectRemoved, 0, 12, RectF{}},
        };
        RecordingCache cache;
        QCOMPARE(applyToCache(changes, 0, cache), std::optional<std::size_t>{0});
        QCOMPARE(cache.calls,
                 (QStringList{QStringLiteral("invalidate 11 page"), QStringLiteral("remove 12 0 0 0 0")}));
    }

    // Une page insérée ou retirée avant la page affichée la déplace : la suite du lot suit son nouvel indice.
    void pageMovesFollowTheShownPage()
    {
        const Changes changes{
            Change{Change::Kind::PageInserted, 0, 0, RectF{}},
            Change{Change::Kind::ObjectInserted, 2, 5, RectF{}},
            Change{Change::Kind::ObjectInserted, 1, 6, RectF{}},
            Change{Change::Kind::PageInserted, 3, 0, RectF{}},
            Change{Change::Kind::PageRemoved, 1, 0, RectF{}},
            Change{Change::Kind::PageRemoved, 0, 0, RectF{}},
            Change{Change::Kind::ObjectInserted, 0, 7, RectF{}},
        };
        RecordingCache cache;
        QCOMPARE(applyToCache(changes, 1, cache), std::optional<std::size_t>{0});
        QCOMPARE(cache.calls, (QStringList{QStringLiteral("insert 5"), QStringLiteral("insert 7")}));
    }

    void removingTheShownPageStopsRouting()
    {
        const Changes changes{
            Change{Change::Kind::ObjectInserted, 1, 5, RectF{}},
            Change{Change::Kind::PageRemoved, 1, 0, RectF{}},
            Change{Change::Kind::ObjectInserted, 1, 6, RectF{}},
        };
        RecordingCache cache;
        QCOMPARE(applyToCache(changes, 1, cache), std::optional<std::size_t>{});
        QCOMPARE(cache.calls, QStringList{QStringLiteral("insert 5")});
    }
};

QTEST_GUILESS_MAIN(TstCorePageCache)
#include "tst_core_pagecache.moc"
