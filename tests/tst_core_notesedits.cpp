#include <actcraft/core/document.h>
#include <actcraft/core/notesedits.h>
#include <actcraft/core/pagecache.h>
#include <actcraft/core/undo.h>

#include <QStringList>
#include <QTest>

#include <chrono>
#include <memory>
#include <optional>
#include <string>

using namespace actcraft::core;
using namespace std::chrono_literals;

namespace {

struct SpyCache final : PageCache
{
    QStringList calls;
    void insert(ObjectId id) override { calls << QStringLiteral("insert %1").arg(id); }
    void remove(ObjectId id, const RectF&) override { calls << QStringLiteral("remove %1").arg(id); }
    void invalidate(ObjectId id, const CacheArea&) override
    {
        calls << QStringLiteral("invalidate %1").arg(id);
    }
};

// Prédicat de presentationTouchesNoCache (L0-S3) : des Change (sinon l'étape n'entre pas dans la pile de
// DocumentController), tous de genre PageSettingsChanged, aucun appel au cache. Vide : respecté.
QString cacheTouches(const Changes& changes)
{
    QStringList faults;
    if (changes.empty()) {
        faults << QStringLiteral("aucun Change");
    }
    for (const Change& change : changes) {
        if (change.kind != Change::Kind::PageSettingsChanged) {
            faults << QStringLiteral("genre %1").arg(int(change.kind));
        }
    }
    SpyCache spy;
    if (applyToCache(changes, 0, spy) != std::optional<std::size_t>{0}) {
        faults << QStringLiteral("applyToCache ne rend pas 0");
    }
    faults << spy.calls;
    return faults.join(u"; ");
}

using Clock = SetPageNotesCommand::Clock;

} // namespace

class TstCoreNotesedits : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<Document> m_document;
    UndoStack m_stack;

    Changes type(int page, const char* html, Clock::time_point when)
    {
        return m_stack.push(*m_document, std::make_unique<SetPageNotesCommand>(page, html, when));
    }

private slots:
    void init()
    {
        m_document = std::make_unique<Document>();
        m_stack.clear();
    }

    void typingMergesIntoOneStep()
    {
        const Clock::time_point t0 = Clock::now();
        type(0, "<p>a</p>", t0);
        type(0, "<p>ab</p>", t0 + 100ms);
        type(0, "<p>abc</p>", t0 + 200ms);
        QCOMPARE(m_stack.count(), std::size_t{1});
        QCOMPARE(m_document->page(0).notes, std::string("<p>abc</p>"));
        type(0, "<p>abcd</p>", t0 + 3200ms); // plus de 2 s après la précédente
        QCOMPARE(m_stack.count(), std::size_t{2});
        m_stack.undo(*m_document);
        QCOMPARE(m_document->page(0).notes, std::string("<p>abc</p>"));
        m_stack.undo(*m_document);
        QCOMPARE(m_document->page(0).notes, std::string());
        m_stack.redo(*m_document);
        QCOMPARE(m_document->page(0).notes, std::string("<p>abc</p>"));
    }

    void otherPageDoesNotMerge()
    {
        QCOMPARE(m_document->appendPage(), std::size_t{1});
        const Clock::time_point t0 = Clock::now();
        type(0, "<p>a</p>", t0);
        type(1, "<p>b</p>", t0 + 100ms);
        QCOMPARE(m_stack.count(), std::size_t{2});
        QCOMPARE(m_document->page(0).notes, std::string("<p>a</p>"));
        QCOMPARE(m_document->page(1).notes, std::string("<p>b</p>"));
    }

    void sameTextChangesNothing()
    {
        const Changes changes = type(0, "", Clock::now());
        QVERIFY(changes.empty());
    }

    void invalidPageChangesNothing()
    {
        QVERIFY(type(5, "<p>x</p>", Clock::now()).empty());
        QVERIFY(type(-1, "<p>x</p>", Clock::now()).empty());
    }

    void notesTouchNoCache()
    {
        const Changes changes = type(0, "<p>x</p>", Clock::now());
        QCOMPARE(cacheTouches(changes), QString());
        QCOMPARE(cacheTouches(m_stack.undo(*m_document)), QString());
        QCOMPARE(cacheTouches(m_stack.redo(*m_document)), QString());
    }
};

QTEST_MAIN(TstCoreNotesedits)
#include "tst_core_notesedits.moc"
