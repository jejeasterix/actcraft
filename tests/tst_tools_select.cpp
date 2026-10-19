#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/lift.h>
#include <actcraft/core/objectedits.h>
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

// Document réel (une page), avec soulèvement comme le contrôleur de ui/ : les objets quittent la page
// affichée hors pile, `drop` les remet puis exécute la commande.
class FakeEditor final : public tools::DocumentEditor
{
public:
    core::Document document;
    std::vector<std::string> calls; // « execute:Transform », « lift », « drop:Remove », « drop:null »…
    int executes = 0;
    int lifts = 0;
    int drops = 0;

    const core::Page& page() const override { return document.page(0); }
    std::size_t pageIndex() const override { return 0; }
    void execute(std::unique_ptr<core::UndoCommand> command) override
    {
        ++executes;
        calls.push_back("execute:" + name(command.get()));
        core::Changes changes;
        command->redo(document, changes);
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
            core::Changes changes;
            command->redo(document, changes);
        }
    }
    bool isLifting() const override { return !m_lifted.empty(); }

    const core::PageObject* find(core::ObjectId id) const
    {
        const core::Page& p = document.page(0);
        const std::size_t index = core::indexOf(p, id);
        return index == p.objects.size() ? nullptr : &p.objects[index];
    }

private:
    static std::string name(const core::UndoCommand* command)
    {
        if (!command) {
            return "null";
        }
        if (dynamic_cast<const core::TransformObjectsCommand*>(command)) {
            return "Transform";
        }
        if (dynamic_cast<const core::RemoveObjectsCommand*>(command)) {
            return "Remove";
        }
        if (dynamic_cast<const core::SetTranslucencyCommand*>(command)) {
            return "Translucency";
        }
        return "other";
    }
    std::vector<core::LiftedObject> m_lifted;
};

class FakeLift final : public tools::LiftSurface
{
public:
    int shows = 0;
    int transforms = 0;
    int clears = 0;
    core::Transform last;
    void show(std::vector<core::PageObject>) override { ++shows; }
    void setTransform(const core::Transform& extra) override
    {
        ++transforms;
        last = extra;
    }
    void clear() override { ++clears; }
};

class FakeSelection final : public tools::SelectionSurface
{
public:
    std::vector<tools::SelectionView> views;
    void show(const tools::SelectionView& view) override { views.push_back(view); }
    const tools::SelectionView& last() const { return views.back(); }
    bool everShowedBand() const
    {
        for (const tools::SelectionView& v : views) {
            if (v.rubberBand) {
                return true;
            }
        }
        return false;
    }
};

class FakeView final : public tools::ViewInfo
{
public:
    double scale = 1.0;
    std::vector<bool> hovers;
    double zoom() const override { return scale; }
    bool overTrash(core::PointF p) const override { return p.x > 1000; }
    void setTrashHover(bool hover) override { hovers.push_back(hover); }
    QStringList objectBarCommands() const override
    {
        return {QStringLiteral("object.move-free"), QStringLiteral("object.rotate"),
                QStringLiteral("object.translucency"), QStringLiteral("object.enlarge"),
                QStringLiteral("object.reduce")};
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

input::Sample sampleAt(Phase phase, double x, double y, std::uint64_t seq = 0)
{
    input::Sample s;
    s.phase = phase;
    s.x = x;
    s.y = y;
    s.seq = seq;
    s.device = input::DeviceKind::Mouse;
    return s;
}

// Tout le montage : une page avec 1 = (100, 100, 100 × 100), 2 = (150, 150, 100 × 100) (recouvre 1), 3 =
// (400, 400, 50 × 50).
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
        core::Page& page = editor.document.page(0);
        page.objects.push_back(rectangle(1, 100, 100, 100, 100));
        page.objects.push_back(rectangle(2, 150, 150, 100, 100));
        page.objects.push_back(rectangle(3, 400, 400, 50, 50));
    }
    void press(double x, double y) { tool.onSample(sampleAt(Phase::Press, x, y)); }
    void move(double x, double y) { tool.onSample(sampleAt(Phase::Move, x, y)); }
    void release(double x, double y) { tool.onSample(sampleAt(Phase::Release, x, y)); }
    void click(double x, double y)
    {
        press(x, y);
        release(x, y);
    }
    // Glisser de (x0, y0) à (x1, y1) en `steps` mouvements, relâché à l'arrivée.
    void drag(double x0, double y0, double x1, double y1, int steps)
    {
        press(x0, y0);
        for (int i = 1; i <= steps; ++i) {
            move(x0 + (x1 - x0) * i / steps, y0 + (y1 - y0) * i / steps);
        }
        release(x1, y1);
    }
    void holdShift(bool on) { tool.onKey(Qt::Key_Shift, on ? int(Qt::ShiftModifier) : 0); }
    void holdCtrl(bool on) { tool.onKey(Qt::Key_Control, on ? int(Qt::ControlModifier) : 0); }

    // Une seule page-objet : 1 = (x, y, w, h), sélectionné ; le cadre est publié.
    void only(double x, double y, double w, double h)
    {
        editor.document.page(0).objects.clear();
        editor.document.page(0).objects.push_back(rectangle(1, x, y, w, h));
        user.setSelection({1});
        tool.refresh();
    }
    core::PageObject& first() { return editor.document.page(0).objects[0]; }
    core::PointF handle(tools::HandleKind kind) const
    {
        return selection.last().handles[static_cast<std::size_t>(kind)];
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
    core::RectF bounds1() const { return core::boundsOnPage(*editor.find(1)); }
};

bool closeTo(double a, double b, double tolerance = 0.01)
{
    return std::abs(a - b) <= tolerance;
}
bool closeRect(const core::RectF& r, double x, double y, double w, double h)
{
    return closeTo(r.x, x) && closeTo(r.y, y) && closeTo(r.width, w) && closeTo(r.height, h);
}
double angleOf(const core::Transform& t)
{
    return std::atan2(t.m12, t.m11) * 180.0 / 3.14159265358979323846;
}
// Point à `degrees` (sens de core::rotation) de `from` autour de `center`.
core::PointF around(core::PointF center, core::PointF from, double degrees)
{
    return core::rotationAbout(center, degrees).map(from);
}

const std::vector<core::ObjectId> none;

} // namespace

class TstToolsSelect : public QObject
{
    Q_OBJECT

private slots:
    void clickSelectsTopmost()
    {
        Rig r;
        r.click(170, 170); // recouvrement de 1 et 2 : 2 est au-dessus
        QCOMPARE(r.user.selection(), (std::vector<core::ObjectId>{2}));
        QVERIFY(r.selection.last().visible);
        QCOMPARE(r.selection.last().corners.size(), std::size_t(4));
        QCOMPARE(r.editor.executes, 0); // la sélection n'est pas une étape
    }

    void clickOnEmptyClears()
    {
        Rig r;
        r.click(170, 170);
        r.click(700, 700);
        QCOMPARE(r.user.selection(), none);
        QVERIFY(!r.selection.last().visible);
    }

    void rubberBandSelectsTouched()
    {
        Rig r;
        r.press(90, 90);
        r.move(110, 110);
        r.move(130, 130);
        QVERIFY(r.selection.last().rubberBand.has_value()); // posé pendant le glisser
        r.release(130, 130);
        QCOMPARE(r.user.selection(), (std::vector<core::ObjectId>{1})); // 2 commence à 150 : pas touché
        QVERIFY(r.selection.everShowedBand());
        QVERIFY(!r.selection.last().rubberBand.has_value()); // effacé au relâché
        QCOMPARE(r.editor.lifts, 0);
    }

    void dragLiftsOnceAndDropsOnce()
    {
        Rig r;
        r.click(120, 120); // sélectionne 1
        r.lift.transforms = r.lift.shows = 0;
        r.drag(120, 120, 220, 120, 10);
        QCOMPARE(r.editor.lifts, 1);
        QCOMPARE(r.editor.executes, 0);
        QCOMPARE(r.lift.shows, 1);
        QCOMPARE(r.lift.transforms, 10);
        QCOMPARE(r.editor.drops, 1);
        QCOMPARE(r.editor.calls.back(), std::string("drop:Transform"));
        const core::PageObject* moved = r.editor.find(1);
        QVERIFY(moved);
        QCOMPARE(moved->common.transform.dx, 100.0);
        QCOMPARE(moved->common.transform.dy, 0.0);
        QCOMPARE(r.lift.clears, 1);
        QCOMPARE(r.user.selection(), (std::vector<core::ObjectId>{1}));
        QVERIFY(!r.tool.isBusy());
    }

    void handlesHiddenAndFrameFollowsWhileDragging()
    {
        Rig r;
        r.click(120, 120);
        r.press(120, 120);
        r.move(170, 120);
        const tools::SelectionView& v = r.selection.last();
        QVERIFY(!v.handlesVisible);      // F-528
        QCOMPARE(v.corners[0].x, 150.0); // cadre de 1 (100) + 50
        r.tool.cancel();
    }

    void clickDoesNotLift()
    {
        Rig r;
        r.click(120, 120);
        QCOMPARE(r.editor.lifts, 0);
        r.press(120, 120);
        r.move(122, 120); // 2 px : pas au-delà du seuil
        r.release(122, 120);
        QCOMPARE(r.editor.lifts, 0);
        QCOMPARE(r.editor.drops, 0);
        QCOMPARE(r.editor.executes, 0);
    }

    void dragStartsBeyondTwoScreenPixels()
    {
        Rig r;
        r.view.scale = 2.0; // 1 px de page = 2 px d'écran
        r.click(120, 120);
        r.press(120, 120);
        r.move(120.9, 120); // 1,8 px d'écran
        QCOMPARE(r.editor.lifts, 0);
        r.move(121.5, 120); // 3 px d'écran
        QCOMPARE(r.editor.lifts, 1);
        r.tool.cancel();
    }

    void lockedObjectDoesNotMove()
    {
        Rig r;
        r.editor.document.page(0).objects[0].common.locked = true;
        r.click(120, 120);
        QCOMPARE(r.user.selection(), (std::vector<core::ObjectId>{1})); // sélectionné
        r.drag(120, 120, 220, 120, 10);
        QCOMPARE(r.editor.lifts, 0);
        QCOMPARE(r.editor.executes, 0);
        QCOMPARE(r.editor.drops, 0);
        QVERIFY(r.editor.find(1)->common.transform.isIdentity());
    }

    void notMovableObjectDoesNotMove()
    {
        Rig r;
        r.editor.document.page(0).objects[0].common.restrictions.movable = false;
        r.click(120, 120);
        r.drag(120, 120, 220, 120, 10);
        QCOMPARE(r.editor.lifts, 0);
        QVERIFY(r.editor.find(1)->common.transform.isIdentity());
    }

    void dropOverTrashDeletes()
    {
        Rig r;
        r.click(420, 420); // sélectionne 3
        r.drag(420, 420, 1100, 420, 10);
        QVERIFY(std::find(r.view.hovers.begin(), r.view.hovers.end(), true) != r.view.hovers.end());
        QCOMPARE(r.editor.calls.back(), std::string("drop:Remove"));
        QVERIFY(r.editor.find(3) == nullptr);
        QCOMPARE(r.user.selection(), none);
        QVERIFY(!r.view.hovers.back()); // la flèche est éteinte
    }

    void escapeCancelsDrag()
    {
        Rig r;
        r.click(120, 120);
        r.press(120, 120);
        r.move(160, 120);
        r.move(200, 120);
        QVERIFY(r.tool.isBusy());
        QVERIFY(r.tool.onKey(Qt::Key_Escape, 0));
        QCOMPARE(r.editor.calls.back(), std::string("drop:null"));
        QVERIFY(!r.tool.isBusy());
        QVERIFY(r.editor.find(1)->common.transform.isIdentity()); // à sa place
        QCOMPARE(r.editor.executes, 0);
        r.release(200, 120); // le relâché qui suit est sans effet
        QCOMPARE(r.editor.drops, 1);
    }

    void deleteWithoutSelectionDoesNothing()
    {
        Rig r;
        r.tool.deleteSelection();
        QVERIFY(!r.tool.onKey(Qt::Key_Delete, 0));
        QCOMPARE(r.editor.executes, 0);
        QCOMPARE(int(r.editor.document.page(0).objects.size()), 3);
    }

    void deleteRemovesSelection()
    {
        Rig r;
        r.click(420, 420);
        QVERIFY(r.tool.onKey(Qt::Key_Delete, 0));
        QCOMPARE(r.editor.calls.back(), std::string("execute:Remove"));
        QVERIFY(r.editor.find(3) == nullptr);
        QCOMPARE(r.user.selection(), none);
    }

    void enlargeScalesAboutTheCenter()
    {
        Rig r;
        r.click(420, 420); // 3 : (400, 400, 50 × 50)
        const core::RectF before = core::boundsOnPage(*r.editor.find(3));
        r.tool.enlarge();
        QCOMPARE(r.editor.calls.back(), std::string("execute:Transform"));
        const core::RectF after = core::boundsOnPage(*r.editor.find(3));
        QVERIFY(std::abs(after.width - before.width * 1.1) < 0.01);
        QVERIFY(std::abs(after.height - before.height * 1.1) < 0.01);
        QVERIFY(std::abs(after.center().x - before.center().x) < 0.01);
        QVERIFY(std::abs(after.center().y - before.center().y) < 0.01);
    }

    void modifierClickTogglesSelection()
    {
        for (const bool useCtrl : {false, true}) {
            Rig r;
            r.click(120, 120); // 1
            if (useCtrl) {
                r.holdCtrl(true);
            } else {
                r.holdShift(true);
            }
            r.click(420, 420); // + 3
            QCOMPARE(r.user.selection(), (std::vector<core::ObjectId>{1, 3}));
            r.click(120, 120); // bascule 1 : retiré
            QCOMPARE(r.user.selection(), (std::vector<core::ObjectId>{3}));
            QCOMPARE(r.editor.lifts, 0);
            r.holdShift(false);
            r.holdCtrl(false);
            r.click(420, 420); // sans touche : sélection de 3 seul, inchangée
            QCOMPARE(r.user.selection(), (std::vector<core::ObjectId>{3}));
        }
    }

    void modifierRubberBandAdds()
    {
        Rig r;
        r.click(420, 420); // 3
        r.holdShift(true);
        r.press(90, 90);
        r.move(110, 110);
        r.move(130, 130);
        r.release(130, 130);
        r.holdShift(false);
        QCOMPARE(r.user.selection(), (std::vector<core::ObjectId>{3, 1})); // ajoute sans vider
        // Sans touche, le même rectangle remplace.
        r.press(90, 90);
        r.move(110, 110);
        r.move(130, 130);
        r.release(130, 130);
        QCOMPARE(r.user.selection(), (std::vector<core::ObjectId>{1}));
    }

    void grabbedUnselectedObjectMovesAndStaysUnselected()
    {
        Rig r;
        r.click(420, 420);             // 3 sélectionné
        r.drag(120, 120, 120, 220, 5); // saisit 1, non sélectionné
        QCOMPARE(r.editor.lifts, 1);
        QCOMPARE(r.editor.calls.back(), std::string("drop:Transform"));
        QCOMPARE(r.editor.find(1)->common.transform.dy, 100.0);
        QVERIFY(r.editor.find(3)->common.transform.isIdentity());       // 3 n'a pas bougé
        QCOMPARE(r.user.selection(), (std::vector<core::ObjectId>{3})); // 1 reste non sélectionné
    }

    void eightHandlesAroundTheFrame()
    {
        Rig r;
        r.only(100, 100, 200, 100);
        const tools::SelectionView& v = r.selection.last();
        const std::vector<core::PointF> expected{{100, 100}, {200, 100}, {300, 100}, {300, 150},
                                                 {300, 200}, {200, 200}, {100, 200}, {100, 150}};
        QCOMPARE(v.handles.size(), expected.size());
        for (std::size_t i = 0; i < expected.size(); ++i) {
            QVERIFY2(closeTo(v.handles[i].x, expected[i].x) && closeTo(v.handles[i].y, expected[i].y),
                     qPrintable(QString::number(i)));
        }
        QCOMPARE(v.bar.size(), std::size_t(5));
        for (const tools::BarSpot& spot : v.bar) {
            QVERIFY(spot.center.y < 100);
        }
        QVERIFY(v.handlesVisible);
    }

    void barMovesBelowWhenItWouldLeaveThePage()
    {
        Rig r;
        r.only(100, 5, 200, 100);
        QCOMPARE(r.selection.last().bar.size(), std::size_t(5));
        for (const tools::BarSpot& spot : r.selection.last().bar) {
            QVERIFY(spot.center.y > 105);
        }
    }

    void barGoesToTheRightOfATallFrame()
    {
        Rig r;
        r.only(100, 100, 50, 200); // plus haut que large
        QCOMPARE(r.selection.last().bar.size(), std::size_t(5));
        for (const tools::BarSpot& spot : r.selection.last().bar) {
            QVERIFY(spot.center.x > 150);
            QVERIFY(spot.center.y >= 100 && spot.center.y <= 300);
        }
        r.only(100, 100, 200, 50); // plus large que haut : au-dessus
        for (const tools::BarSpot& spot : r.selection.last().bar) {
            QVERIFY(spot.center.y < 100);
        }
    }

    void cornerHandleResizesProportionally()
    {
        Rig r;
        r.only(100, 100, 200, 100);
        r.drag(300, 200, 400, 220, 4); // facteurs 1,5 et 1,2 : le plus grand gagne
        QVERIFY(closeRect(r.bounds1(), 100, 100, 300, 150));
        QCOMPARE(r.editor.calls.back(), std::string("drop:Transform"));
    }

    void topLeftCornerResizesFreely()
    {
        Rig r;
        r.only(100, 100, 200, 100);
        r.drag(100, 100, 50, 70, 4); // le coin bas droit reste fixe
        QVERIFY(closeRect(r.bounds1(), 50, 70, 250, 130));
    }

    void sideHandleStretches()
    {
        Rig r;
        r.only(100, 100, 200, 100);
        r.drag(300, 150, 350, 190, 4); // un seul axe
        QVERIFY(closeRect(r.bounds1(), 100, 100, 250, 100));
    }

    void resizeNeverFlipsTheObject()
    {
        Rig r;
        r.only(100, 100, 200, 100);
        r.drag(300, 150, 0, 150, 4); // jusqu'au-delà du côté opposé
        const core::RectF b = r.bounds1();
        QVERIFY(b.width > 0.5 && b.width < 1.5);
        QVERIFY(closeTo(b.x, 100));
        QVERIFY(r.editor.find(1)->common.transform.m11 > 0);
    }

    void rotatedObjectResizesAlongItsOwnAxes()
    {
        Rig r;
        r.only(100, 100, 100, 100);
        r.first().common.transform = core::rotationAbout({150, 150}, 90);
        r.tool.refresh();
        // Poignée droite du cadre pivoté : en bas de la page (150, 200). Glisser vers le bas étire.
        const core::PointF grip = r.handle(tools::HandleKind::Right);
        QVERIFY(closeTo(grip.x, 150) && closeTo(grip.y, 200));
        r.drag(grip.x, grip.y, 150, 250, 4);
        QVERIFY(closeRect(r.bounds1(), 100, 100, 100, 150));
    }

    void rotationAroundCenter()
    {
        Rig r;
        r.only(100, 100, 200, 100);
        const core::PointF center{200, 150};
        const core::PointF grip = r.barSpot(QStringLiteral("object.rotate"));
        r.press(grip.x, grip.y);
        for (const double turn : {30.0, 60.0, 90.0}) {
            const core::PointF p = around(center, grip, turn);
            r.move(p.x, p.y);
        }
        const core::PointF end = around(center, grip, 90);
        r.release(end.x, end.y);
        QCOMPARE(r.editor.executes, 0);
        QCOMPARE(r.editor.drops, 1);
        QCOMPARE(r.editor.calls.back(), std::string("drop:Transform"));
        const core::Transform t = r.editor.find(1)->common.transform;
        QVERIFY(std::abs(angleOf(t) - 90.0) < 0.5);
        const core::PointF mapped = t.map(center);
        QVERIFY(closeTo(mapped.x, 200, 0.5) && closeTo(mapped.y, 150, 0.5)); // le centre ne bouge pas
    }

    void shiftSnapsRotationToFifteenDegrees()
    {
        const std::vector<std::pair<double, double>> cases{{97.0, 90.0}, {20.0, 15.0}, {-40.0, -45.0}};
        for (const auto& [turn, expected] : cases) {
            Rig r;
            r.only(100, 100, 200, 100);
            const core::PointF center{200, 150};
            const core::PointF grip = r.barSpot(QStringLiteral("object.rotate"));
            r.holdShift(true);
            r.press(grip.x, grip.y);
            const core::PointF p = around(center, grip, turn);
            r.move(p.x, p.y);
            r.release(p.x, p.y);
            r.holdShift(false);
            QVERIFY2(std::abs(angleOf(r.editor.find(1)->common.transform) - expected) < 0.01,
                     qPrintable(QString::number(turn)));
        }
    }

    void rotationIsOnlyThroughTheBar()
    {
        Rig r;
        r.only(100, 100, 200, 100);
        // Glisser hors de la barre, hors des poignées, dans le vide : jamais de rotation.
        r.drag(500, 500, 560, 520, 4);
        QVERIFY(r.editor.find(1)->common.transform.isIdentity());
        QCOMPARE(r.editor.lifts, 0);
    }

    void singleRotatedFrameTurnsButMultipleDoesNot()
    {
        Rig r;
        r.editor.document.page(0).objects[0].common.transform = core::rotationAbout({150, 150}, 45);
        const tools::SelectionView one = tools::layoutSelection(r.editor.page(), {1}, 1.0, {});
        QVERIFY(std::abs(one.corners[1].y - one.corners[0].y) > 1.0); // le cadre pivote
        QVERIFY(closeTo(one.corners[0].x, 150) && closeTo(one.corners[0].y, 150 - 50 * std::sqrt(2.0)));
        const tools::SelectionView many = tools::layoutSelection(r.editor.page(), {1, 3}, 1.0, {});
        QCOMPARE(many.corners[0].y, many.corners[1].y); // emprise réunie, jamais pivotée
        QCOMPARE(many.corners[1].x, many.corners[2].x);
        QCOMPARE(many.corners[0].x, many.corners[3].x);
        const core::RectF united = core::unitedRect(core::boundsOnPage(r.editor.page().objects[0]),
                                                    core::boundsOnPage(r.editor.page().objects[2]));
        QCOMPARE(many.corners[0].x, united.x);
        QCOMPARE(many.corners[2].y, united.y + united.height);
    }

    void handlesHiddenWhileDragging()
    {
        Rig r;
        r.only(100, 100, 200, 100);
        QVERIFY(r.selection.last().handlesVisible);
        r.press(300, 200);
        r.move(330, 210);
        r.move(360, 220);
        QVERIFY(!r.selection.last().handlesVisible);
        r.release(360, 220);
        QVERIFY(r.selection.last().handlesVisible);
    }

    void barButtonTriggersCommand()
    {
        Rig r;
        r.only(100, 100, 200, 100);
        const core::PointF button = r.barSpot(QStringLiteral("object.enlarge"));
        r.click(button.x, button.y);
        QCOMPARE(r.commands.triggered, (QStringList{QStringLiteral("object.enlarge")}));
        QCOMPARE(r.editor.lifts, 0);
        QCOMPARE(r.editor.executes, 0);
        QCOMPARE(r.user.selection(), (std::vector<core::ObjectId>{1}));
        QVERIFY(!r.tool.isBusy());
    }

    void restrictionsHonoured()
    {
        {
            Rig r; // non redimensionnable : plus de poignées, la barre reste
            r.only(100, 100, 200, 100);
            r.first().common.restrictions.resizable = false;
            r.tool.refresh();
            QVERIFY(r.selection.last().handles.empty());
            QCOMPARE(r.selection.last().bar.size(), std::size_t(5));
        }
        {
            Rig r; // non pivotable : plus de bouton de rotation
            r.only(100, 100, 200, 100);
            r.first().common.restrictions.rotatable = false;
            r.tool.refresh();
            QCOMPARE(r.selection.last().handles.size(), std::size_t(8));
            QCOMPARE(r.selection.last().bar.size(), std::size_t(4));
            QVERIFY(r.barSpot(QStringLiteral("object.rotate")).x < -1e8);
        }
        {
            Rig r; // verrouillé : « déplacer librement » le déplace quand même
            r.only(100, 100, 200, 100);
            r.first().common.locked = true;
            r.tool.refresh();
            const core::PointF grip = r.barSpot(QStringLiteral("object.move-free"));
            r.drag(grip.x, grip.y, grip.x + 100, grip.y, 5);
            QCOMPARE(r.editor.calls.back(), std::string("drop:Transform"));
            QVERIFY(closeTo(r.editor.find(1)->common.transform.dx, 100.0));
        }
    }

    void lockedObjectNeitherResizesNorRotates()
    {
        Rig r;
        r.only(100, 100, 200, 100);
        r.first().common.locked = true;
        r.tool.refresh();
        QVERIFY(r.selection.last().handles.empty());
        QVERIFY(r.barSpot(QStringLiteral("object.rotate")).x < -1e8);
        r.drag(300, 200, 400, 220, 4);
        QCOMPARE(r.editor.lifts, 0);
        QVERIFY(r.editor.find(1)->common.transform.isIdentity());
    }

    void escapeCancelsResize()
    {
        Rig r;
        r.only(100, 100, 200, 100);
        r.press(300, 200);
        r.move(350, 210);
        r.move(400, 220);
        QVERIFY(r.tool.isBusy());
        QVERIFY(r.tool.onKey(Qt::Key_Escape, 0));
        QCOMPARE(r.editor.calls.back(), std::string("drop:null"));
        QVERIFY(r.editor.find(1)->common.transform.isIdentity());
        QCOMPARE(r.editor.executes, 0);
        QVERIFY(!r.tool.isBusy());
    }

    void resizeLiftsOnceAndDropsOnce()
    {
        Rig r;
        r.only(100, 100, 200, 100);
        r.click(300, 200); // un clic sur une poignée ne soulève rien
        QCOMPARE(r.editor.lifts, 0);
        QCOMPARE(r.editor.drops, 0);
        r.drag(300, 200, 400, 220, 5);
        QCOMPARE(r.editor.lifts, 1);
        QCOMPARE(r.editor.executes, 0);
        QCOMPARE(r.editor.drops, 1);
        QCOMPARE(r.lift.shows, 1);
        QCOMPARE(r.lift.transforms, 5);
        QCOMPARE(r.lift.clears, 1);
        QCOMPARE(r.user.selection(), (std::vector<core::ObjectId>{1}));
        QVERIFY(!r.tool.isBusy());
    }

    void handleGrabRadiusIsInScreenPixels()
    {
        Rig r;
        r.only(100, 100, 200, 100);
        for (const double zoom : {1.0, 2.0}) {
            const tools::SelectionView v = tools::layoutSelection(r.editor.page(), {1}, zoom, {});
            const double inside = 5.5 / zoom;  // 5,5 px d'écran : saisi
            const double outside = 7.0 / zoom; // 7 px d'écran : non
            QCOMPARE(int(tools::hitSelection(v, {300 + inside, 200}, zoom).kind),
                     int(tools::SelectionHitKind::Handle));
            QVERIFY(tools::hitSelection(v, {300 + outside, 200}, zoom).kind !=
                    tools::SelectionHitKind::Handle);
        }
    }
};

QTEST_GUILESS_MAIN(TstToolsSelect)
#include "tst_tools_select.moc"
