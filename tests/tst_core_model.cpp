#include <actcraft/core/clock.h>
#include <actcraft/core/model.h>

#include <QTest>

using namespace actcraft::core;

class TstCoreModel : public QObject
{
    Q_OBJECT

private:
    static Page samplePage()
    {
        Page page;
        Shape shape;
        shape.kind = ShapeKind::Polygon;
        shape.bounds = {10, 20, 30, 40};
        shape.polygon = {{10, 20}, {40, 20}, {25, 60}};
        shape.fill = 0xFF102030u;
        page.objects.emplace_back(shape);
        ImageObject image;
        image.bounds = {100, 100, 64, 48};
        image.pattern = ImagePattern::Checker;
        page.objects.emplace_back(image);
        Stroke stroke;
        stroke.points = {{1, 2, 0.25f}, {3, 4, 0.5f}};
        page.objects.emplace_back(stroke);
        return page;
    }

private slots:
    void kindOfFollowsVariantOrder()
    {
        const Page page = samplePage();
        QCOMPARE(kindOf(page.objects[0]), ObjectKind::Shape);
        QCOMPARE(kindOf(page.objects[1]), ObjectKind::Image);
        QCOMPARE(kindOf(page.objects[2]), ObjectKind::Stroke);
    }

    void defaultPageIs1024By768()
    {
        const Page page;
        QCOMPARE(page.width, 1024.0);
        QCOMPARE(page.height, 768.0);
        QVERIFY(page.objects.empty());
    }

    void fingerprintIsStable() { QCOMPARE(fingerprint(samplePage()), fingerprint(samplePage())); }

    void fingerprintSeesEveryField()
    {
        const std::uint64_t reference = fingerprint(samplePage());
        Page a = samplePage();
        std::get<Stroke>(a.objects[2]).points[1].pressure = 0.5001f;
        QVERIFY(fingerprint(a) != reference);
        Page b = samplePage();
        std::get<Shape>(b.objects[0]).polygon[2].y = 61;
        QVERIFY(fingerprint(b) != reference);
        Page c = samplePage();
        std::get<ImageObject>(c.objects[1]).pattern = ImagePattern::Rings;
        QVERIFY(fingerprint(c) != reference);
        Page d = samplePage();
        std::swap(d.objects[0], d.objects[2]);
        QVERIFY(fingerprint(d) != reference);
    }

    // Valeurs calculées hors C++, à partir du format seul : FNV-1a 64 bits (base 0xCBF29CE484222325,
    // multiplicateur 0x100000001B3) sur les octets petit-boutistes des champs, dans l'ordre de model.cpp.
    // std::hash, un memcpy des structures (pointeurs, bourrage) ou un autre ordre d'octets donneraient
    // d'autres valeurs, et pas les mêmes sous MSVC et sous Clang.
    void fingerprintMatchesIndependentReference()
    {
        QCOMPARE(fingerprint(Page{}), std::uint64_t{0xD93C4EE35CD928ADull});
        QCOMPARE(fingerprint(samplePage()), std::uint64_t{0x21FF926A276797BCull});
    }

    void strokeWidthFollowsPressure()
    {
        QCOMPARE(strokeWidthAt(3.0, 0.5f), 3.0);
        QCOMPARE(strokeWidthAt(3.0, 0.0f), 1.5);
        QCOMPARE(strokeWidthAt(3.0, 1.0f), 4.5);
    }

    void rectContainsAndCenter()
    {
        const RectF r{10, 20, 30, 40};
        QVERIFY(r.contains({10, 20}));
        QVERIFY(r.contains({40, 60}));
        QVERIFY(!r.contains({41, 60}));
        QCOMPARE(r.center().x, 25.0);
        QCOMPARE(r.center().y, 40.0);
    }

    void clockIsMonotonic()
    {
        const std::int64_t a = monotonicNowNs();
        const std::int64_t b = monotonicNowNs();
        QVERIFY(b >= a);
        QVERIFY(a > 0);
    }
};

QTEST_GUILESS_MAIN(TstCoreModel)
#include "tst_core_model.moc"
