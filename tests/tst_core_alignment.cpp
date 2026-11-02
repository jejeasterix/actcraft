#include <actcraft/core/alignment.h>
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/undo.h>

#include <QTest>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace core = actcraft::core;

namespace {

constexpr double kPi = 3.14159265358979323846;

core::Document blank()
{
    return core::Document(); // une page de 1024 × 768
}

core::Transform rotationAbout(double degrees, core::PointF c)
{
    const double r = degrees * kPi / 180.0;
    core::Transform t;
    t.m11 = std::cos(r);
    t.m12 = std::sin(r);
    t.m21 = -std::sin(r);
    t.m22 = std::cos(r);
    t.dx = c.x - (t.m11 * c.x + t.m21 * c.y);
    t.dy = c.y - (t.m12 * c.x + t.m22 * c.y);
    return t;
}

core::Transform scalingAbout(double factor, core::PointF c)
{
    core::Transform t;
    t.m11 = factor;
    t.m22 = factor;
    t.dx = c.x - factor * c.x;
    t.dy = c.y - factor * c.y;
    return t;
}

core::ObjectId addRect(core::Document& d, core::RectF bounds)
{
    core::Shape s;
    s.bounds = bounds;
    s.outlineWidth = 0; // l'emprise ne déborde pas du rectangle
    core::PageObject o = s;
    o.common.id = d.allocateId();
    d.page(0).objects.push_back(std::move(o));
    return d.page(0).objects.back().common.id;
}

core::ObjectId addRotatedRect(core::Document& d, core::RectF bounds, double degrees)
{
    const core::ObjectId id = addRect(d, bounds);
    d.page(0).objects.back().common.transform = rotationAbout(degrees, bounds.center());
    return id;
}

core::PageObject& objectById(core::Document& d, core::ObjectId id)
{
    auto& page = d.page(0);
    return page.objects[core::indexOf(page, id)];
}

core::RectF boundsOf(const core::PageObject& o)
{
    return core::boundsOnPage(o);
}

// Emprise de l'objet une fois `absolute` posée comme sa transformation.
core::RectF boundsWith(core::Document& d, core::ObjectId id, const core::Transform& absolute)
{
    core::PageObject copy = objectById(d, id);
    copy.common.transform = absolute;
    return core::boundsOnPage(copy);
}

double rotationOf(const core::Transform& t)
{
    return std::atan2(t.m12, t.m11) * 180.0 / kPi;
}

// Transformation proposée pour `id`, ou l'identité si absente (le test échoue alors sur son QVERIFY).
const core::Transform* find(const core::TransformList& list, core::ObjectId id)
{
    for (const auto& entry : list) {
        if (entry.first == id) {
            return &entry.second;
        }
    }
    return nullptr;
}

} // namespace

class TestAlignment : public QObject
{
    Q_OBJECT
private slots:
    void alignLeftUsesTheLeftmostEdge()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 10, 50, 50});
        const auto b = addRect(d, {300, 80, 20, 20});
        const auto t = core::alignTransforms(d.page(0), {a, b}, core::AlignMode::Left);
        QCOMPARE(t.size(), std::size_t(1)); // a ne bouge pas
        QCOMPARE(t[0].first, b);
        QCOMPARE(boundsWith(d, b, t[0].second).x, 100.0);
    }
    void alignWidthUsesTheFirstCreated()
    {
        auto d = blank();
        const auto a = addRect(d, {0, 0, 80, 40}); // créé d'abord
        const auto b = addRect(d, {200, 0, 20, 20});
        const auto t = core::alignTransforms(d.page(0), {b, a}, core::AlignMode::Width);
        QVERIFY(find(t, b) != nullptr);
        QCOMPARE(boundsWith(d, b, *find(t, b)).width, 80.0);
        QCOMPARE(boundsWith(d, b, *find(t, b)).x, 200.0); // bord gauche gardé
    }
    void lockedObjectsStay()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 0, 10, 10});
        const auto b = addRect(d, {300, 0, 10, 10});
        objectById(d, b).common.locked = true;
        QVERIFY(core::alignTransforms(d.page(0), {a, b}, core::AlignMode::Left).empty());
        const auto c = addRect(d, {500, 0, 10, 10});
        const auto t = core::alignTransforms(d.page(0), {a, b, c}, core::AlignMode::Left);
        QCOMPARE(t.size(), std::size_t(1)); // b verrouillé reste en place
        QCOMPARE(t[0].first, c);
    }
    void singleObjectIsNotAligned() // D6
    {
        auto d = blank();
        const auto a = addRect(d, {100, 10, 50, 50});
        for (const auto mode : {core::AlignMode::Left, core::AlignMode::CenterX, core::AlignMode::Width,
                                core::AlignMode::Angle, core::AlignMode::All}) {
            QVERIFY(core::alignTransforms(d.page(0), {a}, mode).empty());
        }
        QVERIFY(core::alignTransforms(d.page(0), {a, a}, core::AlignMode::Left).empty());
    }
    void fitOnlyShrinksOversized()
    {
        auto d = blank(); // 1024 × 768
        const auto big = addRect(d, {0, 0, 2048, 300});
        const auto small = addRect(d, {0, 400, 100, 100});
        const auto t = core::fitTransforms(d.page(0), {big, small}, core::FitMode::Width);
        QCOMPARE(t.size(), std::size_t(1));
        QCOMPARE(t[0].first, big);
        QCOMPARE(boundsWith(d, big, t[0].second).width, 1024.0);
        QCOMPARE(boundsWith(d, big, t[0].second).height, 150.0); // proportions gardées
    }
    void flipXNegatesRotation()
    {
        auto d = blank();
        const auto a = addRotatedRect(d, {100, 100, 60, 20}, 30.0);
        const auto t = core::flipTransforms(d.page(0), {a}, core::Axis::X);
        QVERIFY(find(t, a) != nullptr);
        QCOMPARE(rotationOf(*find(t, a)), -30.0);
        const auto after = boundsWith(d, a, *find(t, a)).center();
        const auto before = boundsOf(objectById(d, a)).center();
        QCOMPARE(after.x, before.x);
        QCOMPARE(after.y, before.y);
    }
    void mirrorXPlacesCopyAbove()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 300, 60, 40});
        const auto copies = core::mirrorCopies(d.page(0), {a}, core::Axis::X);
        QCOMPARE(copies.size(), std::size_t(1));
        const auto r = boundsOf(copies[0]);
        QCOMPARE(r.y + r.height, 300.0); // bord contre bord, au-dessus
        QCOMPARE(r.x, 100.0);
        QCOMPARE(r.height, 40.0);
    }
    void mirrorYPlacesCopyOnTheLeft()
    {
        auto d = blank();
        const auto a = addRect(d, {300, 100, 60, 40});
        const auto copies = core::mirrorCopies(d.page(0), {a}, core::Axis::Y);
        QCOMPARE(copies.size(), std::size_t(1));
        const auto r = boundsOf(copies[0]);
        QCOMPARE(r.x + r.width, 300.0);
        QCOMPARE(r.y, 100.0);
    }
    void originSymmetry()
    {
        auto d = blank();
        const auto a = addRect(d, {300, 100, 20, 20});
        const auto t = core::originFlipTransforms(d.page(0), {a}, core::Axis::Y, {200, 0});
        QVERIFY(find(t, a) != nullptr);
        QCOMPARE(boundsWith(d, a, *find(t, a)).x, 80.0); // symétrique de [300, 320] par x = 200
        const auto u = core::originFlipTransforms(d.page(0), {a}, core::Axis::X, {0, 50});
        QCOMPARE(boundsWith(d, a, *find(u, a)).y, -20.0); // symétrique de [100, 120] par y = 50
    }
    void transformEachIsOneStep()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 10, 50, 50});
        const auto b = addRect(d, {300, 80, 20, 20});
        const auto c = addRect(d, {100, 500, 10, 10});
        const auto before = boundsOf(objectById(d, b));
        core::TransformList list;
        list.emplace_back(a, core::translation(-40, 0));
        list.emplace_back(b, core::translation(5, 0));
        list.emplace_back(c, core::Transform{}); // inchangé : aucun Change
        core::UndoStack stack;
        const auto changes = stack.push(d, std::make_unique<core::TransformEachCommand>(0, list));
        QCOMPARE(stack.count(), std::size_t(1));
        QCOMPARE(changes.size(), std::size_t(2));
        QCOMPARE(std::count_if(changes.begin(), changes.end(),
                               [](const auto& ch) { return ch.kind == core::Change::Kind::ObjectChanged; }),
                 2);
        const auto& chB =
            *std::find_if(changes.begin(), changes.end(), [b](const auto& ch) { return ch.object == b; });
        QCOMPARE(chB.area.x, 300.0); // union avant {300..320} et après {5 + 300 ..}
        QCOMPARE(chB.area.width, 25.0);
        QCOMPARE(boundsOf(objectById(d, a)).x, 60.0);
        QCOMPARE(boundsOf(objectById(d, b)).x, 305.0);
        stack.undo(d);
        QCOMPARE(boundsOf(objectById(d, a)).x, 100.0);
        QCOMPARE(boundsOf(objectById(d, b)).x, before.x);
        stack.redo(d);
        QCOMPARE(boundsOf(objectById(d, a)).x, 60.0);
    }

    // Une fonction par mode restant. a = {100, 10, 50, 50} créé d'abord, b = {300, 80, 20, 20} ; l'union
    // couvre x 100..320 et y 10..100.
    void alignCenterX()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 10, 50, 50});
        const auto b = addRect(d, {300, 80, 20, 20});
        const auto t = core::alignTransforms(d.page(0), {a, b}, core::AlignMode::CenterX);
        QVERIFY(find(t, a) && find(t, b));
        QCOMPARE(boundsWith(d, a, *find(t, a)).center().x, 210.0);
        QCOMPARE(boundsWith(d, b, *find(t, b)).center().x, 210.0);
    }
    void alignRight()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 10, 50, 50});
        const auto b = addRect(d, {300, 80, 20, 20});
        const auto t = core::alignTransforms(d.page(0), {a, b}, core::AlignMode::Right);
        QVERIFY(find(t, a));
        QCOMPARE(boundsWith(d, a, *find(t, a)).x + 50.0, 320.0);
        QVERIFY(!find(t, b));
    }
    void alignTop()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 10, 50, 50});
        const auto b = addRect(d, {300, 80, 20, 20});
        const auto t = core::alignTransforms(d.page(0), {a, b}, core::AlignMode::Top);
        QVERIFY(find(t, b));
        QCOMPARE(boundsWith(d, b, *find(t, b)).y, 10.0);
        QVERIFY(!find(t, a));
    }
    void alignCenterY()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 10, 50, 50});
        const auto b = addRect(d, {300, 80, 20, 20});
        const auto t = core::alignTransforms(d.page(0), {a, b}, core::AlignMode::CenterY);
        QVERIFY(find(t, a) && find(t, b));
        QCOMPARE(boundsWith(d, a, *find(t, a)).center().y, 55.0);
        QCOMPARE(boundsWith(d, b, *find(t, b)).center().y, 55.0);
    }
    void alignBottom()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 10, 50, 50});
        const auto b = addRect(d, {300, 80, 20, 20});
        const auto t = core::alignTransforms(d.page(0), {a, b}, core::AlignMode::Bottom);
        QVERIFY(find(t, a));
        const auto r = boundsWith(d, a, *find(t, a));
        QCOMPARE(r.y + r.height, 100.0);
        QVERIFY(!find(t, b));
    }
    void alignHeight()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 10, 50, 50});
        const auto b = addRect(d, {300, 80, 20, 20});
        const auto t = core::alignTransforms(d.page(0), {a, b}, core::AlignMode::Height);
        QVERIFY(find(t, b));
        const auto r = boundsWith(d, b, *find(t, b));
        QCOMPARE(r.height, 50.0);
        QCOMPARE(r.y, 80.0); // haut gardé
        QCOMPARE(r.width, 20.0);
    }
    void alignAngle()
    {
        auto d = blank();
        const auto a = addRotatedRect(d, {100, 10, 50, 50}, 30.0);
        const auto b = addRect(d, {300, 80, 20, 20});
        const auto t = core::alignTransforms(d.page(0), {a, b}, core::AlignMode::Angle);
        QVERIFY(find(t, b));
        QCOMPARE(rotationOf(*find(t, b)), 30.0);
        const auto c = boundsWith(d, b, *find(t, b)).center();
        QCOMPARE(c.x, 310.0); // centre gardé
        QCOMPARE(c.y, 90.0);
    }
    void alignAll()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 10, 50, 50});
        const auto b = addRect(d, {300, 80, 20, 20});
        const auto t = core::alignTransforms(d.page(0), {a, b}, core::AlignMode::All);
        QVERIFY(find(t, b));
        const auto r = boundsWith(d, b, *find(t, b));
        QCOMPARE(r.width, 50.0);
        QCOMPARE(r.height, 50.0);
        QCOMPARE(r.center().x, 125.0);
        QCOMPARE(r.center().y, 35.0);
        QCOMPARE(rotationOf(*find(t, b)), 0.0 + rotationOf(objectById(d, a).common.transform));
    }
    void fitHeight()
    {
        auto d = blank();
        const auto a = addRect(d, {0, 0, 300, 1536});
        const auto t = core::fitTransforms(d.page(0), {a}, core::FitMode::Height);
        QVERIFY(find(t, a));
        const auto r = boundsWith(d, a, *find(t, a));
        QCOMPARE(r.height, 768.0);
        QCOMPARE(r.width, 150.0);
    }
    void fitPage()
    {
        auto d = blank();
        const auto a = addRect(d, {0, 0, 2048, 1536});
        const auto t = core::fitTransforms(d.page(0), {a}, core::FitMode::Page);
        QVERIFY(find(t, a));
        const auto r = boundsWith(d, a, *find(t, a));
        QCOMPARE(r.width, 1024.0);
        QCOMPARE(r.height, 768.0);
        QCOMPARE(r.x, 0.0 + r.x); // dans la page
        QVERIFY(r.x >= -1e-9 && r.y >= -1e-9);
    }
    void originalSizeKeepsTheCenter()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 10, 50, 50});
        objectById(d, a).common.transform = scalingAbout(2.0, {125, 35});
        const auto bigBounds = boundsOf(objectById(d, a));
        QCOMPARE(bigBounds.width, 100.0);
        const auto t = core::fitTransforms(d.page(0), {a}, core::FitMode::OriginalSize);
        QVERIFY(find(t, a));
        const auto r = boundsWith(d, a, *find(t, a));
        QCOMPARE(r.width, 50.0);
        QCOMPARE(r.height, 50.0);
        QCOMPARE(r.center().x, bigBounds.center().x);
        QCOMPARE(r.center().y, bigBounds.center().y);
        // Un objet déjà à sa taille d'origine : rien à faire.
        const auto b = addRect(d, {300, 80, 20, 20});
        QVERIFY(core::fitTransforms(d.page(0), {b}, core::FitMode::OriginalSize).empty());
    }
};

QTEST_GUILESS_MAIN(TestAlignment)
#include "tst_core_alignment.moc"
