#include <actcraft/core/clipboardedits.h>
#include <actcraft/core/connectors.h>
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/lift.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/core/shapeedits.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/core/shapepoints.h>
#include <actcraft/core/undo.h>
#include <actcraft/tools/selectiongeometry.h>
#include <actcraft/tools/selecttool.h>
#include <actcraft/tools/userstate.h>

#include <QTest>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace actcraft;
using input::Phase;

namespace {

// Document réel (une page) et vraie pile d'annulation ; soulèvement comme le contrôleur de ui/ : les objets
// quittent la page affichée hors pile, `drop` les remet puis pousse la commande (une étape).
class FakeEditor final : public tools::DocumentEditor
{
public:
    core::Document document;
    core::UndoStack stack;
    std::vector<std::string> calls; // « execute:Paste », « lift », « drop:Paste », « drop:null »…
    int executes = 0;
    int lifts = 0;
    int drops = 0;

    const core::Page& page() const override { return document.page(0); }
    std::size_t pageIndex() const override { return 0; }
    void execute(std::unique_ptr<core::UndoCommand> command) override
    {
        ++executes;
        calls.push_back("execute:" + name(command.get()));
        stack.push(document, std::move(command));
    }
    std::vector<core::PageObject> lift(const std::vector<core::ObjectId>& ids) override
    {
        ++lifts;
        calls.emplace_back("lift");
        std::vector<core::LiftedObject> taken = core::takeObjects(document.page(0), ids);
        std::vector<core::PageObject> copies;
        for (const core::LiftedObject& item : taken) {
            copies.push_back(item.object);
            m_lifted.push_back(item);
        }
        return copies;
    }
    void drop(std::unique_ptr<core::UndoCommand> command) override
    {
        ++drops;
        calls.push_back("drop:" + name(command.get()));
        core::restoreObjects(document.page(0), m_lifted);
        m_lifted.clear();
        if (command) {
            stack.push(document, std::move(command));
        }
    }
    bool isLifting() const override { return !m_lifted.empty(); }

private:
    static std::string name(const core::UndoCommand* command)
    {
        if (!command) {
            return "null";
        }
        if (dynamic_cast<const core::TransformObjectsCommand*>(command)) {
            return "Transform";
        }
        if (dynamic_cast<const core::PasteObjectsCommand*>(command)) {
            return "Paste";
        }
        if (dynamic_cast<const core::MoveShapePointCommand*>(command)) {
            return "MoveShapePoint";
        }
        if (dynamic_cast<const core::FollowingConnectorsCommand*>(command)) {
            return "FollowingConnectors";
        }
        if (dynamic_cast<const core::RemoveObjectsCommand*>(command)) {
            return "Remove";
        }
        return "other";
    }
    std::vector<core::LiftedObject> m_lifted;
};

class FakeLift final : public tools::LiftSurface
{
public:
    int shows = 0;
    int clears = 0;
    std::vector<core::PageObject> shown;
    void show(std::vector<core::PageObject> objects) override
    {
        ++shows;
        shown = std::move(objects);
    }
    void setTransform(const core::Transform&) override {}
    void clear() override { ++clears; }
};

class FakeSelection final : public tools::SelectionSurface
{
public:
    std::vector<tools::SelectionView> views;
    void show(const tools::SelectionView& view) override { views.push_back(view); }
    const tools::SelectionView& last() const { return views.back(); }
};

class FakeView final : public tools::ViewInfo
{
public:
    double zoom() const override { return 1.0; }
    bool overTrash(core::PointF p) const override { return p.x > 1000; }
    void setTrashHover(bool) override {}
    QStringList objectBarCommands() const override
    {
        return {QStringLiteral("object.move-free"), QStringLiteral("object.rotate"),
                QStringLiteral("object.enlarge")};
    }
};

class FakeInk final : public tools::InkSurface
{
public:
    void beginStroke(core::Argb, double) override {}
    void addPoint(const core::StrokePoint&, std::uint64_t) override {}
    void clear() override {}
};

class FakeStrokes final : public tools::DocumentSink
{
public:
    void commitStroke(core::Stroke) override {}
};

class FakeCommands final : public tools::CommandSink
{
public:
    QStringList triggered;
    bool trigger(const QString& id, const QVariant&) override
    {
        triggered.push_back(id);
        return true;
    }
};

core::PageObject rectangle(core::ObjectId id, double x, double y, double w, double h)
{
    core::Shape shape;
    shape.bounds = {x, y, w, h};
    shape.outlineWidth = 0;
    core::PageObject object{shape};
    object.common.id = id;
    return object;
}

input::Sample sampleAt(Phase phase, double x, double y)
{
    input::Sample s;
    s.phase = phase;
    s.x = x;
    s.y = y;
    s.device = input::DeviceKind::Mouse;
    return s;
}

bool closeTo(double a, double b, double tolerance = 0.01)
{
    return std::abs(a - b) <= tolerance;
}
bool closePoint(core::PointF p, double x, double y)
{
    return closeTo(p.x, x) && closeTo(p.y, y);
}
bool closeRect(const core::RectF& r, double x, double y, double w, double h)
{
    return closeTo(r.x, x) && closeTo(r.y, y) && closeTo(r.width, w) && closeTo(r.height, h);
}
bool samePoints(const std::vector<core::PointF>& a, const std::vector<core::PointF>& b)
{
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (!closePoint(a[i], b[i].x, b[i].y)) {
            return false;
        }
    }
    return true;
}

// Montage : 101 = (100, 100, 100 × 100), 102 = (400, 100, 50 × 50). Identifiants hors de ceux que le document
// tire (1, 2…) : une copie n'en heurte aucun.
struct Rig
{
    FakeEditor editor;
    FakeInk ink;
    FakeStrokes strokes;
    FakeLift lift;
    FakeSelection selection;
    FakeView view;
    FakeCommands commands;
    tools::UserState user;
    tools::ToolContext context{editor, ink, strokes, lift, selection, view, commands, user};
    tools::SelectTool tool{context};

    Rig()
    {
        page().objects.push_back(rectangle(101, 100, 100, 100, 100));
        page().objects.push_back(rectangle(102, 400, 100, 50, 50));
    }
    core::Page& page() { return editor.document.page(0); }
    const core::PageObject* find(core::ObjectId id)
    {
        const std::size_t index = core::indexOf(page(), id);
        return index == page().objects.size() ? nullptr : &page().objects[index];
    }
    core::PageObject& object(core::ObjectId id) { return page().objects[core::indexOf(page(), id)]; }
    core::RectF bounds(core::ObjectId id) { return core::boundsOnPage(object(id)); }
    // Objets ajoutés par l'outil (ni 101, ni 102, ni le connecteur 1000).
    std::vector<core::ObjectId> added()
    {
        std::vector<core::ObjectId> out;
        for (const core::PageObject& o : page().objects) {
            if (o.common.id != 101 && o.common.id != 102 && o.common.id != 1000) {
                out.push_back(o.common.id);
            }
        }
        return out;
    }
    void select(std::vector<core::ObjectId> ids)
    {
        user.setSelection(std::move(ids));
        tool.refresh();
    }
    void press(double x, double y) { tool.onSample(sampleAt(Phase::Press, x, y)); }
    void move(double x, double y) { tool.onSample(sampleAt(Phase::Move, x, y)); }
    void release(double x, double y) { tool.onSample(sampleAt(Phase::Release, x, y)); }
    void drag(double x0, double y0, double x1, double y1, int steps)
    {
        press(x0, y0);
        for (int i = 1; i <= steps; ++i) {
            move(x0 + (x1 - x0) * i / steps, y0 + (y1 - y0) * i / steps);
        }
        release(x1, y1);
    }
    core::PointF barSpot(const QString& command) const
    {
        for (const tools::BarSpot& spot : selection.last().bar) {
            if (spot.command == command) {
                return spot.center;
            }
        }
        return {-1e9, -1e9};
    }
    std::size_t pointNear(double x, double y) const
    {
        const std::vector<core::PointF>& points = selection.last().shapePoints;
        for (std::size_t i = 0; i < points.size(); ++i) {
            if (closePoint(points[i], x, y)) {
                return i;
            }
        }
        return points.size();
    }
};

} // namespace

class TstToolsSelectCopy : public QObject
{
    Q_OBJECT
private slots:
    // D9, F-587 : l'original reste, une copie est déposée là où le glissement finit ; le dépôt passe par le
    // même chemin qu'au lot 3 (drop, jamais execute), avec une PasteObjectsCommand (L5-T4).
    void dragCopyLeavesTheOriginal()
    {
        Rig r;
        r.object(101).common.dragCopy = true;
        r.drag(150, 150, 350, 170, 10);
        QCOMPARE(r.editor.lifts, 1);
        QCOMPARE(r.editor.executes, 0);
        QCOMPARE(r.editor.drops, 1);
        QCOMPARE(r.editor.calls.back(), std::string("drop:Paste"));
        QVERIFY(!r.editor.isLifting()); // l'original n'est pas laissé soulevé
        QVERIFY(!r.tool.isBusy());
        QCOMPARE(r.lift.clears, 1);
        QVERIFY(closeRect(r.bounds(101), 100, 100, 100, 100)); // l'original n'a pas bougé
        QVERIFY(r.find(101)->common.dragCopy);
        const std::vector<core::ObjectId> added = r.added();
        QCOMPARE(added.size(), std::size_t(1));
        QVERIFY(closeRect(r.bounds(added[0]), 300, 120, 100, 100));
        QVERIFY(closeRect(r.bounds(102), 400, 100, 50, 50));
    }

    // Une étape par glissement : un Annuler retire la copie, l'original n'a pas bougé.
    void dragCopyIsOneStep()
    {
        Rig r;
        r.object(101).common.dragCopy = true;
        r.drag(150, 150, 350, 150, 10);
        QCOMPARE(r.editor.stack.count(), std::size_t(1));
        QCOMPARE(r.page().objects.size(), std::size_t(3));
        r.drag(150, 150, 150, 350, 10); // chaque glissement dépose une copie
        QCOMPARE(r.editor.stack.count(), std::size_t(2));
        QCOMPARE(r.page().objects.size(), std::size_t(4));
        r.editor.stack.undo(r.editor.document);
        QCOMPARE(r.page().objects.size(), std::size_t(3));
        r.editor.stack.undo(r.editor.document);
        QCOMPARE(r.page().objects.size(), std::size_t(2));
        QVERIFY(r.added().empty());
        QVERIFY(closeRect(r.bounds(101), 100, 100, 100, 100));
        QVERIFY(!r.editor.stack.canUndo());
    }

    // Sélection mêlée : l'objet ordinaire se déplace, l'objet à copier reste et laisse une copie ; une étape.
    void dragCopyWithAnOrdinaryObjectIsOneStep()
    {
        Rig r;
        r.object(101).common.dragCopy = true;
        r.select({101, 102});
        r.drag(150, 150, 150, 300, 10);
        QCOMPARE(r.editor.lifts, 1);
        QCOMPARE(r.editor.executes, 0);
        QCOMPARE(r.editor.drops, 1);
        QCOMPARE(r.editor.stack.count(), std::size_t(1));
        QVERIFY(closeRect(r.bounds(101), 100, 100, 100, 100));
        QVERIFY(closeRect(r.bounds(102), 400, 250, 50, 50));
        const std::vector<core::ObjectId> added = r.added();
        QCOMPARE(added.size(), std::size_t(1));
        QVERIFY(closeRect(r.bounds(added[0]), 100, 250, 100, 100));
        r.editor.stack.undo(r.editor.document);
        QVERIFY(r.added().empty());
        QVERIFY(closeRect(r.bounds(101), 100, 100, 100, 100));
        QVERIFY(closeRect(r.bounds(102), 400, 100, 50, 50));
    }

    // D9 : verrouillé, seule la poignée « Déplacer » (object.move-free) le déplace ; ni copie, ni point de forme.
    void lockedMovesOnlyByMoveHandle()
    {
        Rig r;
        r.object(101).common.locked = true;
        r.object(101).common.dragCopy = true;
        r.select({101});
        r.drag(150, 150, 250, 150, 5);
        QCOMPARE(r.editor.lifts, 0);
        QCOMPARE(r.editor.drops, 0);
        QVERIFY(r.added().empty());
        QVERIFY(closeRect(r.bounds(101), 100, 100, 100, 100));

        r.tool.setShapePointsMode(true);
        QCOMPARE(r.selection.last().shapePoints.size(), std::size_t(8));
        r.drag(200, 100, 240, 60, 5); // un sommet : sans effet sur un verrouillé
        QCOMPARE(r.editor.lifts, 0);
        QCOMPARE(r.editor.drops, 0);
        QVERIFY(closeRect(r.bounds(101), 100, 100, 100, 100));
        r.tool.setShapePointsMode(false);

        const core::PointF spot = r.barSpot(QStringLiteral("object.move-free"));
        QVERIFY(spot.x > -1e8);
        r.drag(spot.x, spot.y, spot.x + 100, spot.y, 5);
        QCOMPARE(r.editor.lifts, 1);
        QCOMPARE(r.editor.drops, 1);
        QCOMPARE(r.editor.calls.back(), std::string("drop:Transform")); // la poignée déplace, sans copie
        QVERIFY(r.added().empty());
        QVERIFY(closeRect(r.bounds(101), 200, 100, 100, 100));
    }

    // F-527 : en mode points de forme, la forme sélectionnée seule montre ses points (sommets puis milieux).
    void shapePointsModeShowsSquares()
    {
        Rig r;
        r.select({101});
        QVERIFY(r.selection.last().shapePoints.empty()); // hors mode : aucun point
        QVERIFY(!r.tool.shapePointsMode());
        r.tool.setShapePointsMode(true);
        QVERIFY(r.tool.shapePointsMode());
        QCOMPARE(r.selection.last().shapePoints.size(), std::size_t(8)); // 4 sommets, 4 milieux
        QVERIFY(samePoints(r.selection.last().shapePoints, core::shapePoints(r.object(101))));
        QVERIFY(!r.selection.last().handlesVisible); // les carrés remplacent les poignées

        r.select({101, 102}); // deux objets : aucun point
        QVERIFY(r.selection.last().shapePoints.empty());
        r.select({101});
        QCOMPARE(r.selection.last().shapePoints.size(), std::size_t(8));

        // Une forme intelligente : ses carrés, pas ses points intelligents.
        core::PageObject arrow = core::makeShapeObject("arrow", {100, 300, 200, 100}, core::ShapeStyle{});
        arrow.common.id = 103;
        r.page().objects.push_back(std::move(arrow));
        r.select({103});
        QVERIFY(r.selection.last().smartPoints.empty());
        QVERIFY(samePoints(r.selection.last().shapePoints, core::shapePoints(r.object(103))));

        QVERIFY(r.tool.onKey(Qt::Key_Escape, 0)); // Échap quitte le mode
        QVERIFY(!r.tool.shapePointsMode());
        QVERIFY(r.selection.last().shapePoints.empty());
        QVERIFY(!r.tool.onKey(Qt::Key_Escape, 0)); // au repos, hors mode : comme au lot 3
    }

    // Glisser un point : aperçu par la couche soulevée, une MoveShapePointCommand au relâché, enveloppée par
    // withFollowingConnectors (sa fusion perdue) : une seule étape, le connecteur lié suit le même sommet.
    void shapePointDragIsOneStep()
    {
        Rig r;
        core::PageObject link = core::makeConnector({200, 100}, 101, {400, 40}, 0, 0);
        link.common.id = 1000;
        r.page().objects.push_back(std::move(link));
        r.select({101});
        r.tool.setShapePointsMode(true);
        const std::vector<core::PointF> before = r.selection.last().shapePoints;
        const std::size_t corner = r.pointNear(200, 100);
        QVERIFY(corner < before.size());

        r.lift.shows = 0;
        r.drag(200, 100, 240, 60, 5);
        QCOMPARE(r.editor.lifts, 1);
        QCOMPARE(r.editor.executes, 0);
        QCOMPARE(r.editor.drops, 1);
        QCOMPARE(r.editor.stack.count(), std::size_t(1));
        QVERIFY(r.lift.shows >= 5); // l'aperçu suit chaque mouvement
        QVERIFY(!r.editor.isLifting());
        QVERIFY(!r.tool.isBusy());
        QVERIFY(r.tool.shapePointsMode());
        QVERIFY(closePoint(core::shapePoints(r.object(101))[corner], 240, 60));
        QVERIFY(closePoint(r.selection.last().shapePoints[corner], 240, 60));
        // Ordre des sommets stable (D13) : le connecteur suit le même point d'attache.
        QVERIFY(closePoint(std::get<core::Connector>(core::body(r.object(1000))).firstEnd, 240, 60));

        r.editor.stack.undo(r.editor.document);
        QVERIFY(samePoints(core::shapePoints(r.object(101)), before));
        QVERIFY(closePoint(std::get<core::Connector>(core::body(r.object(1000))).firstEnd, 200, 100));

        // Un clic sur un point ne soulève rien.
        r.tool.refresh();
        const core::PointF grab = r.selection.last().shapePoints[corner];
        r.press(grab.x, grab.y);
        r.release(grab.x, grab.y);
        QCOMPARE(r.editor.lifts, 1);
        QCOMPARE(r.editor.drops, 1);
    }

    // H5-14 : une forme intelligente dont on déplace un point perd son préréglage (règle de L5-T9, pas de l'outil).
    void smartShapeLosesItsPresetWhenAPointMoves()
    {
        Rig r;
        core::PageObject arrow = core::makeShapeObject("arrow", {100, 300, 200, 100}, core::ShapeStyle{});
        arrow.common.id = 103;
        r.page().objects.push_back(std::move(arrow));
        r.select({103});
        r.tool.setShapePointsMode(true);
        QVERIFY(!r.selection.last().shapePoints.empty());
        const core::PointF grab = r.selection.last().shapePoints.front();
        r.drag(grab.x, grab.y, grab.x - 20, grab.y - 20, 4);
        QCOMPARE(r.editor.stack.count(), std::size_t(1));
        QVERIFY(std::get<core::Shape>(core::body(r.object(103))).preset.empty());
        QVERIFY(r.selection.last().smartPoints.empty());
        QVERIFY(closePoint(r.selection.last().shapePoints.front(), grab.x - 20, grab.y - 20));
    }
};

QTEST_GUILESS_MAIN(TstToolsSelectCopy)
#include "tst_tools_selectcopy.moc"
