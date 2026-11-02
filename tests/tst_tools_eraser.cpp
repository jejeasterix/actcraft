#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/lift.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/core/replace.h>
#include <actcraft/tools/erasertool.h>
#include <actcraft/tools/userstate.h>

#include <QTest>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

using namespace actcraft;
using input::Phase;

namespace {

// Doublures de toutes les interfaces de ToolContext.
class FakeEditor final : public tools::DocumentEditor
{
public:
    explicit FakeEditor(core::Document& document) : m_document(document) {}

    std::vector<core::ObjectId> liftedIds; // chaque identifiant demandé à lift(), dans l'ordre
    int liftCalls = 0;
    std::vector<core::UndoCommand*> executed;             // execute() : doit rester vide
    std::vector<core::UndoCommand*> dropped;              // drop() avec commande
    int dropCalls = 0;                                    // drop() avec ou sans commande
    int dropNullCalls = 0;                                // drop(nullptr)
    std::vector<std::unique_ptr<core::UndoCommand>> keep; // commandes déposées, gardées pour annuler

    const core::Page& page() const override { return m_document.page(0); }
    std::size_t pageIndex() const override { return 0; }
    void execute(std::unique_ptr<core::UndoCommand> command) override
    {
        executed.push_back(command.get());
        keep.push_back(std::move(command));
    }
    std::vector<core::PageObject> lift(const std::vector<core::ObjectId>& ids) override
    {
        ++liftCalls;
        liftedIds.insert(liftedIds.end(), ids.begin(), ids.end());
        std::vector<core::LiftedObject> taken = core::takeObjects(m_document.page(0), ids);
        std::vector<core::PageObject> copies;
        for (const core::LiftedObject& l : taken)
            copies.push_back(l.object);
        m_batches.push_back(std::move(taken));
        return copies;
    }
    void drop(std::unique_ptr<core::UndoCommand> command) override
    {
        ++dropCalls;
        for (auto it = m_batches.rbegin(); it != m_batches.rend(); ++it)
            core::restoreObjects(m_document.page(0), *it);
        m_batches.clear();
        if (!command) {
            ++dropNullCalls;
            return;
        }
        core::Changes changes;
        command->redo(m_document, changes);
        dropped.push_back(command.get());
        keep.push_back(std::move(command));
    }
    bool isLifting() const override { return !m_batches.empty(); }
    core::UndoCommand* lastCommand() const { return keep.empty() ? nullptr : keep.back().get(); }

private:
    core::Document& m_document;
    std::vector<std::vector<core::LiftedObject>> m_batches;
};

class FakeLift final : public tools::LiftSurface
{
public:
    int showCalls = 0;
    int clearCalls = 0;
    std::vector<core::PageObject> shown;
    void show(std::vector<core::PageObject> objects) override
    {
        ++showCalls;
        shown = std::move(objects);
    }
    void setTransform(const core::Transform&) override {}
    void clear() override
    {
        ++clearCalls;
        shown.clear();
    }
};

class FakeSelection final : public tools::SelectionSurface
{
public:
    void show(const tools::SelectionView&) override {}
};

class FakeView final : public tools::ViewInfo
{
public:
    double zoom() const override { return 1; }
    bool overTrash(core::PointF) const override { return false; }
    void setTrashHover(bool) override {}
    QStringList objectBarCommands() const override { return {}; }
};

class FakeCommands final : public tools::CommandSink
{
public:
    bool trigger(const QString&, const QVariant&) override { return false; }
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

input::Sample sample(Phase phase, double x, double y)
{
    input::Sample s;
    s.x = x;
    s.y = y;
    s.phase = phase;
    s.device = input::DeviceKind::Mouse;
    return s;
}

struct Point
{
    double x;
    double y;
};

// Points régulièrement espacés de `from` à `to`, en `steps` mouvements (steps + 1 points).
std::vector<Point> line(Point from, Point to, int steps)
{
    std::vector<Point> out;
    for (int i = 0; i <= steps; ++i) {
        const double t = static_cast<double>(i) / steps;
        out.push_back({from.x + t * (to.x - from.x), from.y + t * (to.y - from.y)});
    }
    return out;
}

void press(tools::Tool& tool, Point p)
{
    tool.onSample(sample(Phase::Press, p.x, p.y));
}

void moveAlong(tools::Tool& tool, const std::vector<Point>& points)
{
    for (std::size_t i = 1; i < points.size(); ++i)
        tool.onSample(sample(Phase::Move, points[i].x, points[i].y));
}

void release(tools::Tool& tool, Point p)
{
    tool.onSample(sample(Phase::Release, p.x, p.y));
}

// Appui au premier point, mouvements, relâché au dernier.
void drag(tools::Tool& tool, const std::vector<Point>& points)
{
    press(tool, points.front());
    moveAlong(tool, points);
    release(tool, points.back());
}

core::ObjectId addStroke(core::Document& doc, core::Layer layer, Point a, Point b, double width = 3,
                         bool locked = false)
{
    core::Stroke stroke;
    stroke.points = {{a.x, a.y, 0.5f}, {b.x, b.y, 0.5f}};
    stroke.baseWidth = width;
    core::PageObject object{stroke};
    object.common.id = doc.allocateId();
    object.common.layer = layer;
    object.common.locked = locked;
    core::insertAtTopOfLayer(doc.page(0), object);
    return object.common.id;
}

core::ObjectId addRectangle(core::Document& doc, core::RectF bounds)
{
    core::Shape shape;
    shape.bounds = bounds;
    core::PageObject object{shape};
    object.common.id = doc.allocateId();
    object.common.layer = core::Layer::Middle;
    core::insertAtTopOfLayer(doc.page(0), object);
    return object.common.id;
}

std::size_t strokeCount(const core::Document& doc)
{
    const auto& objects = doc.page(0).objects;
    return static_cast<std::size_t>(
        std::count_if(objects.begin(), objects.end(), [](const core::PageObject& o) {
            return std::get_if<core::Stroke>(&core::body(o)) != nullptr;
        }));
}

bool pageHas(const core::Document& doc, core::ObjectId id)
{
    return core::indexOf(doc.page(0), id) != doc.page(0).objects.size();
}

bool wasLifted(const FakeEditor& editor, core::ObjectId id)
{
    return std::find(editor.liftedIds.begin(), editor.liftedIds.end(), id) != editor.liftedIds.end();
}

// Un tout monté : document, doublures, outil. Deux traits horizontaux (y 100 et 200) de x 100 à 300.
struct Rig
{
    core::Document doc;
    FakeEditor editor{doc};
    FakeLift lift;
    FakeSelection selection;
    FakeView view;
    FakeCommands commands;
    FakeInk ink;
    FakeStrokes strokes;
    tools::UserState user;
    tools::ToolContext context{editor, ink, strokes, lift, selection, view, commands, user};
    tools::EraserTool tool{context};
    core::ObjectId upper = 0;
    core::ObjectId lower = 0;

    Rig()
    {
        upper = addStroke(doc, core::Layer::Top, {100, 100}, {300, 100});
        lower = addStroke(doc, core::Layer::Top, {100, 200}, {300, 200});
    }
    std::vector<Point> verticalPass() const { return line({200, 50}, {200, 250}, 20); }
};

} // namespace

class TstToolsEraser : public QObject
{
    Q_OBJECT

private slots:
    void liftsEachTouchedStrokeOnce()
    {
        Rig rig;
        drag(rig.tool, rig.verticalPass());
        QCOMPARE(rig.editor.liftCalls, 2);
        QCOMPARE(rig.editor.liftedIds.size(), std::size_t{2});
        QVERIFY(wasLifted(rig.editor, rig.upper));
        QVERIFY(wasLifted(rig.editor, rig.lower));
        QVERIFY(rig.editor.liftedIds[0] != rig.editor.liftedIds[1]);
    }

    void nothingExecutedDuringTheGesture()
    {
        Rig rig;
        const std::vector<Point> pass = rig.verticalPass();
        press(rig.tool, pass.front());
        moveAlong(rig.tool, pass);
        QVERIFY(rig.tool.isBusy());
        QVERIFY(rig.editor.executed.empty());
        QVERIFY(rig.editor.dropped.empty());
        QCOMPARE(rig.editor.dropCalls, 0);
        rig.tool.cancel();
    }

    void oneCommandAtRelease()
    {
        Rig rig;
        drag(rig.tool, rig.verticalPass());
        QCOMPARE(rig.editor.dropCalls, 1);
        QCOMPARE(rig.editor.dropped.size(), std::size_t{1});
        QVERIFY(rig.editor.executed.empty());
        QVERIFY(dynamic_cast<core::ReplaceObjectsCommand*>(rig.editor.dropped.front()) != nullptr);
        QCOMPARE(strokeCount(rig.doc), std::size_t{4});
        QVERIFY(!pageHas(rig.doc, rig.upper));
        QVERIFY(!pageHas(rig.doc, rig.lower));
        QVERIFY(!rig.tool.isBusy());
        QVERIFY(rig.lift.clearCalls >= 1);
    }

    void shapesAreNeverTouched()
    {
        Rig rig;
        const core::ObjectId rect = addRectangle(rig.doc, {150, 50, 100, 200});
        drag(rig.tool, rig.verticalPass());
        QVERIFY(pageHas(rig.doc, rect));
        QVERIFY(!wasLifted(rig.editor, rect));
    }

    void movedAnnotationIsNotErased()
    {
        Rig rig;
        const core::ObjectId moved = addStroke(rig.doc, core::Layer::Middle, {100, 150}, {300, 150});
        drag(rig.tool, rig.verticalPass());
        QVERIFY(pageHas(rig.doc, moved));
        QVERIFY(!wasLifted(rig.editor, moved));
    }

    // P3 (D4) : un trait verrouillé n'est ni soulevé ni coupé.
    void lockedStrokeIsSkipped()
    {
        Rig rig;
        const core::ObjectId locked = addStroke(rig.doc, core::Layer::Top, {100, 150}, {300, 150}, 3, true);
        drag(rig.tool, rig.verticalPass());
        QVERIFY(pageHas(rig.doc, locked));
        QVERIFY(!wasLifted(rig.editor, locked));
        QCOMPARE(rig.editor.liftCalls, 2); // les deux autres traits, pas celui-ci
        QCOMPARE(strokeCount(rig.doc), std::size_t{5});
    }

    // Décision de l'orchestrateur : on n'agit pas sur ce qu'on ne voit pas (comme isSelectable).
    void hiddenStrokeIsSkipped()
    {
        Rig rig;
        const core::ObjectId hidden = addStroke(rig.doc, core::Layer::Top, {100, 150}, {300, 150});
        rig.doc.page(0).objects[core::indexOf(rig.doc.page(0), hidden)].common.visible = false;
        drag(rig.tool, rig.verticalPass());
        QVERIFY(pageHas(rig.doc, hidden));
        QVERIFY(!wasLifted(rig.editor, hidden));
        QCOMPARE(rig.editor.liftCalls, 2);
        QCOMPARE(strokeCount(rig.doc), std::size_t{5});
    }

    void cancelRestoresEverything()
    {
        Rig rig;
        const std::uint64_t before = core::fingerprint(rig.doc.page(0));
        const std::vector<Point> pass = rig.verticalPass();
        press(rig.tool, pass.front());
        moveAlong(rig.tool, pass);
        QVERIFY(rig.editor.isLifting());
        rig.tool.cancel();
        QCOMPARE(rig.editor.dropCalls, 1);
        QCOMPARE(rig.editor.dropNullCalls, 1);
        QVERIFY(rig.editor.dropped.empty());
        QVERIFY(rig.lift.clearCalls >= 1);
        QVERIFY(!rig.tool.isBusy());
        QCOMPARE(core::fingerprint(rig.doc.page(0)), before);
    }

    void pieceCountAfterMiddleCut()
    {
        Rig rig;
        press(rig.tool, {200, 100});
        release(rig.tool, {200, 100});
        QCOMPARE(rig.editor.dropped.size(), std::size_t{1});
        QCOMPARE(strokeCount(rig.doc), std::size_t{3}); // 2 morceaux + le trait du bas
        QVERIFY(!pageHas(rig.doc, rig.upper));
    }

    void liftLayerShowsTheCutPieces()
    {
        Rig rig;
        press(rig.tool, {200, 100});
        rig.tool.onSample(sample(Phase::Move, 201, 100));
        QVERIFY(rig.lift.showCalls >= 1);
        QCOMPARE(rig.lift.shown.size(), std::size_t{2});
        for (const core::PageObject& piece : rig.lift.shown) {
            QVERIFY(core::isAnnotation(piece));
            QVERIFY(piece.common.transform.isIdentity());
        }
        rig.tool.cancel();
    }

    void radiusIsHalfTheConfiguredWidth()
    {
        Rig rig;
        QCOMPARE(rig.tool.radius(), 25.0); // gomme de 50 par défaut (D4)
        rig.user.setSettings(QStringLiteral("eraser"), tools::InkSettings{0xFF000000u, 20, 0});
        QCOMPARE(rig.tool.radius(), 10.0);
    }

    void idIsEraser()
    {
        Rig rig;
        QCOMPARE(rig.tool.id(), QStringLiteral("eraser"));
    }

    void isBusyDuringTheGesture()
    {
        Rig rig;
        QVERIFY(!rig.tool.isBusy());
        press(rig.tool, {10, 10});
        QVERIFY(rig.tool.isBusy());
        release(rig.tool, {10, 10});
        QVERIFY(!rig.tool.isBusy());
    }

    void cancelWhenIdleDoesNothing()
    {
        Rig rig;
        rig.tool.cancel();
        QCOMPARE(rig.editor.dropCalls, 0);
        QCOMPARE(rig.lift.clearCalls, 0);
    }

    void untouchedStrokeIsNotLifted()
    {
        Rig rig;
        drag(rig.tool, line({200, 400}, {200, 500}, 10));
        QCOMPARE(rig.editor.liftCalls, 0);
        QCOMPARE(rig.editor.dropCalls, 0); // rien de soulevé : rien
        QCOMPARE(strokeCount(rig.doc), std::size_t{2});
    }

    // Soulevé (l'emprise touche le balayage) mais la ligne centrale est hors du disque : pas de remplacement.
    void liftedButUncutStrokeIsPutBackAsIs()
    {
        Rig rig;
        const core::ObjectId fat = addStroke(rig.doc, core::Layer::Top, {100, 400}, {300, 400}, 40);
        const std::uint64_t before = core::fingerprint(rig.doc.page(0));
        drag(rig.tool, line({150, 440}, {250, 440}, 10)); // 40 sous la ligne, disque de 25
        QVERIFY(wasLifted(rig.editor, fat));
        QCOMPARE(rig.editor.dropNullCalls, 1);
        QVERIFY(rig.editor.dropped.empty());
        QCOMPARE(core::fingerprint(rig.doc.page(0)), before);
    }

    void undoAfterReleaseRestoresThePage()
    {
        Rig rig;
        const std::uint64_t before = core::fingerprint(rig.doc.page(0));
        drag(rig.tool, rig.verticalPass());
        QVERIFY(core::fingerprint(rig.doc.page(0)) != before);
        core::Changes changes;
        rig.editor.lastCommand()->undo(rig.doc, changes);
        QCOMPARE(core::fingerprint(rig.doc.page(0)), before);
    }

    void sampleWithoutPressIsIgnored()
    {
        Rig rig;
        rig.tool.onSample(sample(Phase::Move, 200, 100));
        rig.tool.onSample(sample(Phase::Release, 200, 100));
        QCOMPARE(rig.editor.liftCalls, 0);
        QCOMPARE(rig.editor.dropCalls, 0);
        QVERIFY(!rig.tool.isBusy());
    }

    void twoGesturesGiveTwoCommands()
    {
        Rig rig;
        drag(rig.tool, line({150, 50}, {150, 150}, 10));
        drag(rig.tool, line({250, 150}, {250, 250}, 10));
        QCOMPARE(rig.editor.dropped.size(), std::size_t{2});
        QCOMPARE(rig.editor.dropCalls, 2);
        QVERIFY(!rig.editor.isLifting());
    }

    void strokeSwallowedWholeDisappears()
    {
        Rig rig;
        const core::ObjectId tiny = addStroke(rig.doc, core::Layer::Top, {500, 500}, {510, 500});
        press(rig.tool, {505, 500});
        release(rig.tool, {505, 500});
        QVERIFY(!pageHas(rig.doc, tiny));
        QCOMPARE(strokeCount(rig.doc), std::size_t{2});
    }

    void verticesOnTheCircleLeaveTwoPieces()
    {
        // Constat de S6 (L3-C6) : trait y = 300 de x 200 à 600 en 16 sous-segments de 25 px, gomme de rayon
        // 25 passée en x = 400 de y 260 à 340 par mouvements de 10 px ; au centre (400, 300), les sommets 375
        // et 425 sont pile sur le cercle. Les deux morceaux restent : [200, 375] et [425, 600].
        Rig rig;
        core::Stroke stroke;
        for (int x = 200; x <= 600; x += 25)
            stroke.points.push_back({static_cast<double>(x), 300, 0.5f});
        stroke.baseWidth = 3;
        core::PageObject object{stroke};
        const core::ObjectId id = object.common.id = rig.doc.allocateId();
        object.common.layer = core::Layer::Top;
        core::insertAtTopOfLayer(rig.doc.page(0), object);

        drag(rig.tool, line({400, 260}, {400, 340}, 8));
        QCOMPARE(rig.editor.dropped.size(), std::size_t{1});
        QVERIFY(!pageHas(rig.doc, id));
        QCOMPARE(strokeCount(rig.doc), std::size_t{4}); // 2 morceaux + les deux traits du banc
        std::vector<std::pair<double, double>> spans;
        for (const core::PageObject& o : rig.doc.page(0).objects) {
            const auto* s = std::get_if<core::Stroke>(&core::body(o));
            if (s && !s->points.empty() && s->points.front().y == 300)
                spans.emplace_back(s->points.front().x, s->points.back().x);
        }
        std::sort(spans.begin(), spans.end());
        QCOMPARE(spans.size(), std::size_t{2});
        QVERIFY(qAbs(spans[0].first - 200) < 1e-9 && qAbs(spans[0].second - 375) < 1e-9);
        QVERIFY(qAbs(spans[1].first - 425) < 1e-9 && qAbs(spans[1].second - 600) < 1e-9);
    }

    // M1 (L3-C7) : après un soulèvement, des mouvements loin de tout morceau ne coupent ni ne remontrent
    // rien.
    void movesInTheVoidCutAndShowNothing()
    {
        Rig rig;
        press(rig.tool, {200, 100});
        rig.tool.onSample(sample(Phase::Move, 200, -50)); // remonte par le trou qu'il vient de faire
        QVERIFY(rig.editor.isLifting());
        QVERIFY(rig.lift.showCalls >= 1);
        const int shows = rig.lift.showCalls;
        const std::size_t cuts = rig.tool.cutCalls();
        moveAlong(rig.tool, line({200, -50}, {200, -250}, 20));
        QCOMPARE(rig.lift.showCalls, shows);
        QCOMPARE(rig.tool.cutCalls(), cuts);
        QCOMPARE(rig.lift.shown.size(), std::size_t{2}); // les deux morceaux restent montrés
        rig.tool.cancel();
    }

    // M1 (L3-C7) : le coût d'un mouvement suit les morceaux proches de la gomme, pas le nombre de traits
    // soulevés plus tôt dans le geste.
    void cutCallsIgnoreFarLiftedStrokes()
    {
        const auto cutsOfTheLastLeg = [](int k) {
            Rig rig;
            for (int i = 0; i < k; ++i)
                addStroke(rig.doc, core::Layer::Top, {100, 300.0 + 60 * i}, {200, 300.0 + 60 * i});
            const core::ObjectId far = addStroke(rig.doc, core::Layer::Top, {800, 300}, {800, 900});
            press(rig.tool, {150, 300});
            moveAlong(rig.tool, line({150, 300}, {150, 300.0 + 60 * (k - 1)}, 6 * k)); // coupe les k traits
            rig.tool.onSample(sample(Phase::Move, 700, 1000));
            rig.tool.onSample(sample(Phase::Move, 700, 600));
            const std::size_t before = rig.tool.cutCalls();
            moveAlong(rig.tool, line({700, 600}, {900, 600}, 20)); // traverse le trait vertical
            const std::size_t leg = rig.tool.cutCalls() - before;
            release(rig.tool, {900, 600});
            const bool farCut = !pageHas(rig.doc, far);
            return std::pair<std::size_t, bool>{leg, farCut};
        };
        const auto few = cutsOfTheLastLeg(2);
        const auto many = cutsOfTheLastLeg(8);
        QVERIFY(few.second && many.second);
        QVERIFY(few.first > 0);
        QCOMPARE(many.first, few.first);
    }

    // M3 (L3-C7) : les morceaux d'un trait agrandi gardent son épaisseur visible.
    void scaledStrokePiecesKeepTheVisibleWidth()
    {
        Rig rig;
        core::Stroke stroke;
        stroke.points = {{450, 500, 0.5f}, {550, 500, 0.5f}};
        stroke.baseWidth = 4;
        core::PageObject object{stroke};
        const core::ObjectId id = object.common.id = rig.doc.allocateId();
        object.common.layer = core::Layer::Top;
        object.common.transform = core::scalingAbout({500, 500}, 3, 3); // de 350 à 650, épaisseur visible 12
        core::insertAtTopOfLayer(rig.doc.page(0), object);

        press(rig.tool, {500, 500});
        rig.tool.onSample(sample(Phase::Move, 501, 500));
        QCOMPARE(rig.lift.shown.size(), std::size_t{2});
        for (const core::PageObject& piece : rig.lift.shown)
            QCOMPARE(std::get<core::Stroke>(core::body(piece)).baseWidth, 12.0);
        release(rig.tool, {501, 500});

        QVERIFY(!pageHas(rig.doc, id));
        int pieces = 0;
        for (const core::PageObject& o : rig.doc.page(0).objects) {
            const auto* s = std::get_if<core::Stroke>(&core::body(o));
            if (!s || s->points.empty() || s->points.front().y != 500)
                continue;
            ++pieces;
            QVERIFY(o.common.transform.isIdentity());
            QCOMPARE(s->baseWidth, 12.0);
        }
        QCOMPARE(pieces, 2);
    }

    // m6 (L3-C7) : seul le pointeur de l'appui mène le geste ; un appui d'un autre pointeur est ignoré.
    void otherPointerIsIgnored()
    {
        Rig rig;
        input::Sample down = sample(Phase::Press, 200, 100);
        down.pointer = 1;
        rig.tool.onSample(down);
        for (Phase phase : {Phase::Press, Phase::Move, Phase::Release}) {
            input::Sample other = sample(phase, 200, 200);
            other.pointer = 2;
            rig.tool.onSample(other);
        }
        QVERIFY(rig.tool.isBusy());
        QCOMPARE(rig.editor.dropCalls, 0);
        QVERIFY(!wasLifted(rig.editor, rig.lower));
        input::Sample up = sample(Phase::Release, 200, 100);
        up.pointer = 1;
        rig.tool.onSample(up);
        QVERIFY(!rig.tool.isBusy());
        QCOMPARE(rig.editor.dropped.size(), std::size_t{1});
        QVERIFY(pageHas(rig.doc, rig.lower));
        QVERIFY(!pageHas(rig.doc, rig.upper));
    }
};

QTEST_GUILESS_MAIN(TstToolsEraser)
#include "tst_tools_eraser.moc"
