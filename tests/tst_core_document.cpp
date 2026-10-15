#include <actcraft/core/document.h>

#include <QByteArray>
#include <QTest>

using namespace actcraft::core;

namespace {

Stroke redStroke()
{
    Stroke s;
    s.points = {StrokePoint{10, 20, 0.5f}, StrokePoint{30, 40, 0.5f}};
    s.baseWidth = 4;
    s.color = 0xFFE53935u;
    return s;
}

} // namespace

// Squelette du document (DA2) : pages, navigation de F-125, empreinte.
class TstCoreDocument : public QObject
{
    Q_OBJECT

private slots:
    void newDocumentHasOneBlankPage()
    {
        Document d;
        QCOMPARE(d.pageCount(), std::size_t{1});
        QCOMPARE(d.page(0).width, 1024.0);
        QCOMPARE(d.page(0).height, 768.0);
        QVERIFY(d.page(0).objects.empty());
    }

    void nextOnLastPageAppendsBlankPage()
    {
        Document d;
        d.page(0).objects.emplace_back(redStroke());
        QCOMPARE(nextPageIndex(d, 0), std::size_t{1});
        QCOMPARE(d.pageCount(), std::size_t{2});
        QVERIFY(d.page(1).objects.empty());
        QCOMPARE(d.page(0).objects.size(), std::size_t{1});
    }

    void nextInsideDocumentDoesNotAppend()
    {
        Document d;
        d.appendPage();
        QCOMPARE(nextPageIndex(d, 0), std::size_t{1});
        QCOMPARE(d.pageCount(), std::size_t{2});
    }

    void appendedPageKeepsLastPageSize()
    {
        Document d;
        d.page(0).width = 800;
        d.page(0).height = 600;
        QCOMPARE(d.appendPage(), std::size_t{1});
        QCOMPARE(d.page(1).width, 800.0);
        QCOMPARE(d.page(1).height, 600.0);
    }

    void previousStopsAtFirstPage()
    {
        QCOMPARE(previousPageIndex(0), std::size_t{0});
        QCOMPARE(previousPageIndex(3), std::size_t{2});
    }

    void fingerprintFollowsPagesAndContent()
    {
        Document a;
        Document b;
        QCOMPARE(fingerprint(a), fingerprint(b));
        b.appendPage();
        QVERIFY(fingerprint(a) != fingerprint(b));
        Document c;
        c.page(0).objects.emplace_back(redStroke());
        QVERIFY(fingerprint(a) != fingerprint(c));
        Document d;
        d.page(0).objects.emplace_back(redStroke());
        QCOMPARE(fingerprint(c), fingerprint(d));
    }

    void resourcesAreKeyedBySha256AndDeduplicated()
    {
        Document d;
        const std::string key = d.addResource(QByteArray("abc"), "PNG");
        QCOMPARE(QString::fromStdString(key),
                 QStringLiteral("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
        QCOMPARE(d.addResource(QByteArray("abc"), "png"), key);
        QCOMPARE(d.resources().size(), std::size_t{1});
        QVERIFY(d.resource(key) != nullptr);
        QCOMPARE(d.resource(key)->extension, std::string("png"));
        QCOMPARE(d.resource(key)->data, QByteArray("abc"));
        QVERIFY(d.resource("absente") == nullptr);
        // Extension sûre seulement : jamais de chemin dans un nom d'entrée d'archive.
        QCOMPARE(d.resource(d.addResource(QByteArray("y"), "../../evil"))->extension, std::string("bin"));
        QCOMPARE(d.resource(d.addResource(QByteArray("z"), ""))->extension, std::string("bin"));
        QCOMPARE(d.resource(d.addResource(QByteArray("w"), "tropLongueExtension"))->extension,
                 std::string("bin"));
    }

    void pagesCanBeInsertedAndTaken()
    {
        Document d;
        Page wide;
        wide.width = 800;
        d.insertPage(0, wide);
        QCOMPARE(d.pageCount(), std::size_t{2});
        QCOMPARE(d.page(0).width, 800.0);
        const Page taken = d.takePage(0);
        QCOMPARE(taken.width, 800.0);
        QCOMPARE(d.pageCount(), std::size_t{1});
        QCOMPARE(d.page(0).width, 1024.0);
    }

    void documentFromPagesNeverEmpty()
    {
        const Document empty(std::vector<Page>{});
        QCOMPARE(empty.pageCount(), std::size_t{1});
        Page a;
        a.width = 640;
        const Document two(std::vector<Page>{a, Page{}});
        QCOMPARE(two.pageCount(), std::size_t{2});
        QCOMPARE(two.page(0).width, 640.0);
        const Document sized(1280, 1024);
        QCOMPARE(sized.page(0).width, 1280.0);
        QCOMPARE(sized.page(0).height, 1024.0);
    }

    void identifiersAreAllocatedAfterTheLargestInUse()
    {
        Document d;
        QCOMPARE(d.allocateId(), ObjectId{1});
        QCOMPARE(d.allocateId(), ObjectId{2});
        PageObject o = redStroke();
        o.common.id = 40;
        d.page(0).objects.push_back(o);
        d.refreshNextId();
        QCOMPARE(d.nextId(), ObjectId{41});
        d.reserveIdsBelow(100);
        QCOMPARE(d.allocateId(), ObjectId{100});
        d.reserveIdsBelow(50); // jamais en arrière
        QCOMPARE(d.nextId(), ObjectId{101});
    }

    void uniqueIdsAreChecked()
    {
        Document d;
        PageObject a = redStroke();
        a.common.id = 1;
        PageObject b = redStroke();
        b.common.id = 2;
        d.page(0).objects = {a, b};
        QVERIFY(hasUniqueIds(d));
        d.appendPage();
        d.page(1).objects.push_back(a);
        QVERIFY(!hasUniqueIds(d)); // l'identifiant 1 sur deux pages
        d.page(1).objects[0].common.id = 0;
        QVERIFY(!hasUniqueIds(d)); // 0 n'est pas un identifiant
    }

    void fingerprintFollowsResourcesAndTitleButNotNextId()
    {
        Document a;
        Document b;
        b.setTitle("Leçon");
        QVERIFY(fingerprint(a) != fingerprint(b));
        Document c;
        c.addResource(QByteArray("x"), "png");
        QVERIFY(fingerprint(a) != fingerprint(c));
        Document d;
        d.allocateId();
        QCOMPARE(fingerprint(a), fingerprint(d));
    }
};

QTEST_GUILESS_MAIN(TstCoreDocument)
#include "tst_core_document.moc"
