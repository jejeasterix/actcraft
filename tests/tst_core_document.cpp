#include <actcraft/core/document.h>

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
};

QTEST_GUILESS_MAIN(TstCoreDocument)
#include "tst_core_document.moc"
