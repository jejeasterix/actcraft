#include <actcraft/core/clock.h>
#include <actcraft/core/model.h>

#include <QTest>

#include <cmath>
#include <cstdint>
#include <limits>
#include <variant>

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

    // Un objet de chaque type ajouté par le lot 2, champs hors défaut.
    static Page richPage()
    {
        Page page = samplePage();
        Line line;
        line.points = {{0, 0}, {10, 5}};
        line.pen.width = 3;
        page.objects.emplace_back(line);
        Text text;
        text.bounds = {0, 0, 90, 36};
        text.html = "<p>Bonjour</p>";
        TextRun run;
        run.text = "Bonjour";
        run.fontFamily = "Arial";
        run.pointSize = 24;
        text.runs.push_back(run);
        page.objects.emplace_back(text);
        Media media;
        media.bounds = {0, 0, 64, 48};
        media.media = "abc";
        page.objects.emplace_back(media);
        Group group;
        group.members = {1, 2};
        page.objects.emplace_back(group);
        Connector connector;
        connector.endB = {100, 100};
        page.objects.emplace_back(connector);
        page.grid.levels.push_back(GridLevel{});
        return page;
    }

private slots:
    void kindOfFollowsVariantOrder()
    {
        const Page page = richPage();
        QCOMPARE(kindOf(page.objects[0]), ObjectKind::Shape);
        QCOMPARE(kindOf(page.objects[1]), ObjectKind::Image);
        QCOMPARE(kindOf(page.objects[2]), ObjectKind::Stroke);
        QCOMPARE(kindOf(page.objects[3]), ObjectKind::Line);
        QCOMPARE(kindOf(page.objects[4]), ObjectKind::Text);
        QCOMPARE(kindOf(page.objects[5]), ObjectKind::Media);
        QCOMPARE(kindOf(page.objects[6]), ObjectKind::Group);
        QCOMPARE(kindOf(page.objects[7]), ObjectKind::Connector);
        QCOMPARE(std::variant_size_v<ObjectVariant>, kObjectKindCount);
    }

    void pageObjectKeepsVariantAccessAndCommonProperties()
    {
        Page page = samplePage();
        PageObject& object = page.objects[2];
        QVERIFY(std::get_if<Stroke>(&object) != nullptr);
        QVERIFY(std::get_if<Shape>(&object) == nullptr);
        QCOMPARE(object.common.layer, Layer::Middle);
        QCOMPARE(object.common.id, ObjectId{0});
        QVERIFY(object.common.transform.isIdentity());
        object.common.layer = Layer::Top;
        const PageObject copy = object;
        QCOMPARE(copy.common.layer, Layer::Top);
        QCOMPARE(std::get<Stroke>(copy).points.size(), std::size_t{2});
        QCOMPARE(body(copy).index(), std::size_t{2});
    }

    void defaultPageIs1024By768OnWhite()
    {
        const Page page;
        QCOMPARE(page.width, 1024.0);
        QCOMPARE(page.height, 768.0);
        QVERIFY(page.objects.empty());
        QCOMPARE(page.background.fill.kind, FillKind::Solid);
        QCOMPARE(page.background.fill.color, Argb{0xFFFFFFFFu});
        QVERIFY(page.background.image.empty());
        QVERIFY(!page.grid.visible);
        QVERIFY(page.grid.levels.empty());
    }

    void transformsComposeLikeQTransform()
    {
        // Mise à l'échelle puis rotation de 90° (sens horaire, y vers le bas) puis translation.
        const Transform t = compose(compose(scaling(2, 1), rotation(90)), translation(100, 100));
        const PointF p = t.map({100, 0});
        QCOMPARE(p.x, 100.0);
        QCOMPARE(p.y, 300.0);
        const Transform r = rotation(90);
        QCOMPARE(r.m11, 0.0);
        QCOMPARE(r.m12, 1.0);
        QCOMPARE(r.m21, -1.0);
        QCOMPARE(r.m22, 0.0);
        QVERIFY(rotation(360).isIdentity());
        // Pas de −0 : l'empreinte hache le motif binaire, et −0 == 0 cacherait l'écart à isIdentity().
        QVERIFY(!std::signbit(rotation(0).m21));
        QVERIFY(!std::signbit(rotation(180).m21));
        QCOMPARE(rotation(-90).m12, -1.0);
        const PointF q = rotation(30).map({1, 0});
        QVERIFY(qAbs(q.x - 0.8660254037844386) < 1e-12);
        QVERIFY(qAbs(q.y - 0.5) < 1e-12);
    }

    void solidFillSetsKindAndColor()
    {
        const Fill f = solidFill(0xFF00FF00u);
        QCOMPARE(f.kind, FillKind::Solid);
        QCOMPARE(f.color, Argb{0xFF00FF00u});
    }

    void fingerprintIsStable() { QCOMPARE(fingerprint(richPage()), fingerprint(richPage())); }

    void fingerprintSeesEveryField()
    {
        const std::uint64_t reference = fingerprint(richPage());
        const auto differs = [reference](auto mutate) {
            Page p = richPage();
            mutate(p);
            return fingerprint(p) != reference;
        };
        QVERIFY(differs([](Page& p) { std::get<Stroke>(p.objects[2]).points[1].pressure = 0.5001f; }));
        QVERIFY(differs([](Page& p) { std::get<Shape>(p.objects[0]).polygon[2].y = 61; }));
        QVERIFY(differs([](Page& p) { std::get<ImageObject>(p.objects[1]).pattern = ImagePattern::Rings; }));
        QVERIFY(differs([](Page& p) { std::swap(p.objects[0], p.objects[2]); }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.id = 7; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.name = "Forme1"; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.layer = Layer::Bottom; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.visible = false; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.locked = true; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.translucency = 1; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.restrictions.canSize = false; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.transform.dx = 0.5; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.label = "Légende"; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.actions.push_back(Action{}); }));
        QVERIFY(differs([](Page& p) { std::get<Shape>(p.objects[0]).paths.push_back({{1, 1}}); }));
        QVERIFY(
            differs([](Page& p) { std::get<Shape>(p.objects[0]).brush.gradient = GradientKind::Radial; }));
        QVERIFY(differs([](Page& p) { std::get<Shape>(p.objects[0]).pen.join = PenJoin::Bevel; }));
        QVERIFY(differs([](Page& p) { std::get<ImageObject>(p.objects[1]).resource = "x"; }));
        QVERIFY(differs([](Page& p) { std::get<Stroke>(p.objects[2]).kind = StrokeKind::Highlighter; }));
        QVERIFY(differs([](Page& p) { std::get<Line>(p.objects[3]).pen.style = PenStyle::Dash; }));
        QVERIFY(differs([](Page& p) { std::get<Text>(p.objects[4]).html = "<p>Bonsoir</p>"; }));
        QVERIFY(differs([](Page& p) { std::get<Text>(p.objects[4]).runs[0].weight = 700; }));
        QVERIFY(differs([](Page& p) { std::get<Media>(p.objects[5]).loop = true; }));
        QVERIFY(differs([](Page& p) { std::get<Group>(p.objects[6]).members.push_back(3); }));
        QVERIFY(differs([](Page& p) { std::get<Connector>(p.objects[7]).objectA = 9; }));
        QVERIFY(differs([](Page& p) { p.background.fill.color2 = 0xFF000000u; }));
        QVERIFY(differs([](Page& p) { p.background.image = "cle"; }));
        QVERIFY(differs([](Page& p) { p.grid.visible = true; }));
        QVERIFY(differs([](Page& p) { p.grid.levels[0].stepY = 31; }));
        QVERIFY(differs([](Page& p) { p.name = "Page un"; }));
        QVERIFY(differs([](Page& p) { p.notes = "notes"; }));
    }

    // Valeurs recoupées hors C++ par le script Python de L2-T1 (étape 7), qui hache les octets
    // petit-boutistes des champs dans l'ordre de model.cpp : FNV-1a 64 bits, base 0xCBF29CE484222325,
    // multiplicateur 0x100000001B3. Valeurs du lot 1 avant l'ajout des champs du lot 2 : 0xD93C4EE35CD928AD
    // et 0x21FF926A276797BC.
    void fingerprintMatchesIndependentReference()
    {
        QCOMPARE(fingerprint(Page{}), std::uint64_t{0xB59B2DE2A9B3C823ull});
        QCOMPARE(fingerprint(samplePage()), std::uint64_t{0x29E4AA55B1A00D2Eull});
    }

    // −0 et +0 se valent (même rendu) : l'empreinte ne doit pas les distinguer. Un NaN, lui, garde son motif
    // binaire (inchangé) : égal à lui-même, différent de 0.
    void fingerprintTreatsNegativeZeroAsPositiveZero()
    {
        const auto withZero = [](double zero) {
            Page p = richPage();
            std::get<Shape>(p.objects[0]).polygon[0].x = zero;
            std::get<Stroke>(p.objects[2]).points[0].y = zero;
            p.objects[0].common.transform.m12 = zero;
            p.objects[1].common.transform.dx = zero;
            std::get<ImageObject>(p.objects[1]).bounds.x = zero;
            p.grid.levels[0].angle = zero;
            return p;
        };
        const Page positive = withZero(0.0);
        const Page negative = withZero(-0.0);
        QVERIFY(std::signbit(std::get<Shape>(negative.objects[0]).polygon[0].x));
        QCOMPARE(fingerprint(negative), fingerprint(positive));

        const auto withNan = [](double v) {
            Page p = richPage();
            p.objects[0].common.transform.m12 = v;
            return p;
        };
        const double nan = std::numeric_limits<double>::quiet_NaN();
        QCOMPARE(fingerprint(withNan(nan)), fingerprint(withNan(nan)));
        QVERIFY(fingerprint(withNan(nan)) != fingerprint(withNan(0.0)));
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
