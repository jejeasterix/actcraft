#include <actcraft/core/pagecache.h>

#include <QStringList>
#include <QTest>

using namespace actcraft::core;

namespace {

class RecordingCache final : public PageCache
{
public:
    QStringList calls;
    void insert(ObjectId id) override { calls << QStringLiteral("insert %1").arg(id); }
    void remove(ObjectId id) override { calls << QStringLiteral("remove %1").arg(id); }
    void invalidate(const RectF& r) override
    {
        calls << QStringLiteral("invalidate %1 %2").arg(r.width).arg(r.height);
    }
};

} // namespace

// DA11 : les changements de la pile n'atteignent le cache que par son interface, pour la seule page affichée.
class TstCorePageCache : public QObject
{
    Q_OBJECT

private slots:
    void onlyTheShownPageReachesTheCache()
    {
        const Changes changes{
            Change{Change::Kind::ObjectInserted, 1, 7, RectF{0, 0, 10, 10}},
            Change{Change::Kind::ObjectInserted, 0, 8, RectF{}},
            Change{Change::Kind::ObjectRemoved, 1, 9, RectF{}},
            Change{Change::Kind::ObjectChanged, 1, 10, RectF{5, 5, 20, 30}},
            Change{Change::Kind::PageChanged, 1, 0, RectF{}},
            Change{Change::Kind::PageInserted, 1, 0, RectF{}},
            Change{Change::Kind::PageRemoved, 1, 0, RectF{}},
        };
        RecordingCache cache;
        applyToCache(changes, 1, cache);
        QCOMPARE(cache.calls,
                 (QStringList{QStringLiteral("insert 7"), QStringLiteral("remove 9"),
                              QStringLiteral("invalidate 20 30"), QStringLiteral("invalidate 0 0")}));
    }
};

QTEST_GUILESS_MAIN(TstCorePageCache)
#include "tst_core_pagecache.moc"
