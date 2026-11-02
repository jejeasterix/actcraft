#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/shapeedits.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/core/shapepoints.h>
#include <actcraft/core/undo.h>

#include <QString>
#include <QTest>

#include <algorithm>
#include <memory>
#include <vector>

namespace core = actcraft::core;

namespace {

core::Document blank()
{
    return core::Document{};
}

core::ObjectId insert(core::Document& d, core::PageObject o)
{
    o.common.id = d.nextId();
    core::insertAtTopOfLayer(d.page(0), std::move(o));
    d.refreshNextId();
    return d.page(0).objects.back().common.id;
}

core::ObjectId addRect(core::Document& d, core::RectF bounds)
{
    core::Shape s;
    s.bounds = bounds;
    return insert(d, core::PageObject{s});
}

core::ObjectId addSmartShape(core::Document& d, const char* preset)
{
    return insert(d, core::makeShapeObject(preset, {100, 100, 200, 100}, {}));
}

core::PageObject& objectById(core::Document& d, core::ObjectId id)
{
    auto& page = d.page(0);
    return page.objects[core::indexOf(page, id)];
}

bool contains(const std::vector<core::PointF>& points, core::PointF p)
{
    return std::any_of(points.begin(), points.end(),
                       [p](const core::PointF& q) { return q.x == p.x && q.y == p.y; });
}

core::RectF boundsOf(const core::PageObject& o)
{
    return core::boundsOnPage(o);
}

// Empreinte complète de la forme (l'empreinte du modèle ignore le préréglage et ses paramètres).
QString objectFingerprint(const core::PageObject& o)
{
    const auto& s = std::get<core::Shape>(core::body(o));
    QString out = QStringLiteral("%1|%2,%3,%4,%5|%6|%7|").arg(static_cast<int>(s.kind)).arg(s.bounds.x).arg(
        s.bounds.y).arg(s.bounds.width).arg(s.bounds.height).arg(QString::fromStdString(s.preset)).arg(
        s.paths.size());
    for (const auto& p : s.polygon)
        out += QStringLiteral("(%1,%2)").arg(p.x).arg(p.y);
    for (double v : s.parameters)
        out += QStringLiteral("[%1]").arg(v);
    return out;
}

} // namespace

class TestShapePoints : public QObject
{
    Q_OBJECT
private slots:
    void squareHasEightPoints()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 100, 100});
        const auto points = core::shapePoints(objectById(d, a));
        QCOMPARE(points.size(), std::size_t(8));
        QVERIFY(contains(points, {100, 100}) && contains(points, {150, 100}) && contains(points, {200, 200}));
    }
    void movingAPointDeformsTheShape()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 100, 100});
        core::UndoStack stack;
        stack.push(d, std::make_unique<core::MoveShapePointCommand>(0, a, 0, core::PointF{80, 60}));
        QVERIFY(contains(core::shapePoints(objectById(d, a)), {80, 60}));
        QCOMPARE(boundsOf(objectById(d, a)).y, 60.0);
        const auto& shape = std::get<core::Shape>(core::body(objectById(d, a)));
        QCOMPARE(shape.kind, core::ShapeKind::Polygon);
    }
    void smartShapeKeepsItsSmartPoints()
    {
        auto d = blank();
        const auto arrow = addSmartShape(d, "arrow");
        QVERIFY(!core::shapePoints(objectById(d, arrow)).empty());
        const auto& before = std::get<core::Shape>(core::body(objectById(d, arrow)));
        QVERIFY(!core::smartPoints(before).empty()); // tant qu'aucun point de forme n'a bougé
        core::UndoStack stack;
        stack.push(d, std::make_unique<core::MoveShapePointCommand>(0, arrow, 0, core::PointF{90, 90}));
        const auto& after = std::get<core::Shape>(core::body(objectById(d, arrow)));
        QVERIFY(after.preset.empty()); // devenue polygone : elle perd ses paramètres
        QVERIFY(core::smartPoints(after).empty());
        stack.undo(d);
        QVERIFY(!core::smartPoints(std::get<core::Shape>(core::body(objectById(d, arrow)))).empty());
    }
    void pointMoveIsOneStep()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 100, 100});
        const auto before = objectFingerprint(objectById(d, a));
        core::UndoStack stack;
        const auto changes =
            stack.push(d, std::make_unique<core::MoveShapePointCommand>(0, a, 3, core::PointF{250, 250}));
        QCOMPARE(changes.size(), std::size_t(1));
        QCOMPARE(stack.count(), std::size_t(1));
        QVERIFY(objectFingerprint(objectById(d, a)) != before);
        stack.undo(d);
        QCOMPARE(objectFingerprint(objectById(d, a)), before);
        stack.redo(d);
        QVERIFY(contains(core::shapePoints(objectById(d, a)), {250, 250}));
    }
    void movingAMidpointInsertsAVertex()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 100, 100});
        core::UndoStack stack;
        stack.push(d, std::make_unique<core::MoveShapePointCommand>(0, a, 4, core::PointF{150, 80}));
        const auto& shape = std::get<core::Shape>(core::body(objectById(d, a)));
        QCOMPARE(shape.polygon.size(), std::size_t(5));
        QCOMPARE(core::shapePoints(objectById(d, a)).size(), std::size_t(10));
        QVERIFY(contains(core::shapePoints(objectById(d, a)), {150, 80}));
    }
    void transformIsApplied()
    {
        auto d = blank();
        const auto a = addRect(d, {0, 0, 100, 100});
        objectById(d, a).common.transform = core::translation(10, 20);
        QVERIFY(contains(core::shapePoints(objectById(d, a)), {110, 120}));
        core::UndoStack stack;
        stack.push(d, std::make_unique<core::MoveShapePointCommand>(0, a, 0, core::PointF{0, 0}));
        const auto& shape = std::get<core::Shape>(core::body(objectById(d, a)));
        QCOMPARE(shape.polygon.front().x, -10.0); // remis en coordonnées de l'objet
        QCOMPARE(shape.polygon.front().y, -20.0);
    }
    void lockedShapeRefusesTheMove()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 100, 100});
        objectById(d, a).common.locked = true;
        const auto before = objectFingerprint(objectById(d, a));
        core::UndoStack stack;
        const auto changes =
            stack.push(d, std::make_unique<core::MoveShapePointCommand>(0, a, 0, core::PointF{80, 60}));
        QVERIFY(changes.empty());
        QCOMPARE(objectFingerprint(objectById(d, a)), before);
        QVERIFY(objectById(d, a).common.locked);
    }
    void nonShapeHasNoPoints()
    {
        core::Line line;
        line.points = {{0, 0}, {10, 10}};
        QVERIFY(core::shapePoints(core::PageObject{line}).empty());
    }
};

QTEST_GUILESS_MAIN(TestShapePoints)
#include "tst_core_shapepoints.moc"
