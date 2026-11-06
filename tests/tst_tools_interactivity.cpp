#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/lift.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/core/propertyedits.h>
#include <actcraft/core/restrictors.h>
#include <actcraft/tools/interactionpolicy.h>
#include <actcraft/tools/selecttool.h>
#include <actcraft/tools/userstate.h>

#include <QTest>

#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// Lot 8 (L8-T16) : l'outil de sélection applique limiteurs, rotation, objets bloquants et conteneurs pendant
// le geste et au relâché, sans jamais corriger après le dépôt : un retour est un `drop(nullptr)` (aucune
// étape), un dépôt accepté est une seule commande, et rien ne touche la page affichée pendant le glisser.

using namespace actcraft;
using input::Phase;

namespace {

// Document réel (une page), avec soulèvement comme le contrôleur de ui/ (montage de tst_tools_select).
class FakeEditor final : public tools::DocumentEditor
{
public:
    core::Document document;
    std::vector<std::string> calls; // « execute:Transform », « lift », « drop:Transform », « drop:null »
    int executes = 0;
    int lifts = 0;
    int drops = 0;
    std::vector<std::unique_ptr<core::UndoCommand>> dropped; // une étape par dépôt non nul

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
            dropped.push_back(std::move(command));
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
        return "other";
    }
    std::vector<core::LiftedObject> m_lifted;
};

class FakeLift final : public tools::LiftSurface
{
public:
    int shows = 0;
    int transforms = 0;
    core::Transform last;
    void show(std::vector<core::PageObject>) override { ++shows; }
    void setTransform(const core::Transform& extra) override
    {
        ++transforms;
        last = extra;
    }
    void clear() override {}
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
    QStringList triggered; // un retour par « edit.undo » s'y verrait
    bool trigger(const QString& id, const QVariant&) override
    {
        triggered.push_back(id);
        return true;
    }
};

// Mode conception (D8-9) : ce que ui::ToolHost pose par son adaptateur.
class FakePolicy final : public tools::InteractionPolicy
{
public:
    bool actions = true;
    bool restrictions = true;
    bool blockers = true;
    bool actionsEnabled() const override { return actions; }
    bool restrictionsEnabled() const override { return restrictions; }
    bool blockersEnabled() const override { return blockers; }
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

constexpr core::ObjectId kBasket = 10;
constexpr core::ObjectId kApple = 11;
constexpr core::ObjectId kCarrot = 12;

struct Accepted
{
    core::ObjectId container = 0;
    bool chime = false;
};

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
    std::vector<Accepted> accepted;

    Rig()
    {
        tool.setContainerAccepted(
            [this](core::ObjectId container, bool chime) { accepted.push_back({container, chime}); });
    }
    core::Page& page() { return editor.document.page(0); }
    void press(double x, double y) { tool.onSample(sampleAt(Phase::Press, x, y)); }
    void move(double x, double y) { tool.onSample(sampleAt(Phase::Move, x, y)); }
    void release(double x, double y) { tool.onSample(sampleAt(Phase::Release, x, y)); }
    // Appui en (x0, y0), `steps` mouvements jusqu'à (x1, y1), sans relâcher.
    void dragTo(double x0, double y0, double x1, double y1, int steps)
    {
        press(x0, y0);
        for (int i = 1; i <= steps; ++i) {
            move(x0 + (x1 - x0) * i / steps, y0 + (y1 - y0) * i / steps);
        }
    }
    void drag(double x0, double y0, double x1, double y1, int steps)
    {
        dragTo(x0, y0, x1, y1, steps);
        release(x1, y1);
    }
    // Panier « fruit » (L8-T14) derrière une pomme « fruit » et une carotte « légume » : le panier renvoie ce
    // qu'il refuse et sonne à l'acceptation. Centres : panier (400, 200), pomme (70, 70), carotte (70, 220).
    void basket()
    {
        core::PageObject basket = rectangle(kBasket, 300, 100, 200, 200);
        basket.common.container.accepts = core::Accepts::Keywords;
        basket.common.container.keywords = {"fruit"};
        basket.common.container.bounceBack = true;
        basket.common.container.chime = true;
        page().objects.push_back(std::move(basket));
        core::PageObject apple = rectangle(kApple, 50, 50, 40, 40);
        apple.common.keywords = {"fruit"};
        page().objects.push_back(std::move(apple));
        core::PageObject carrot = rectangle(kCarrot, 50, 200, 40, 40);
        carrot.common.keywords = {"legume"};
        page().objects.push_back(std::move(carrot));
    }
    core::PointF centre(core::ObjectId id) const { return core::boundsOnPage(*editor.find(id)).center(); }
    core::RectF bounds(core::ObjectId id) const { return core::boundsOnPage(*editor.find(id)); }
    core::PointF barSpot(const QString& command) const
    {
        for (const tools::BarSpot& spot : selection.last().bar) {
            if (spot.command == command) {
                return spot.center;
            }
        }
        return {-1e9, -1e9};
    }
};

bool closeTo(double a, double b, double tolerance = 0.01)
{
    return std::abs(a - b) <= tolerance;
}
bool closePoint(core::PointF p, double x, double y)
{
    return closeTo(p.x, x) && closeTo(p.y, y);
}
double angleOf(const core::Transform& t)
{
    return std::atan2(t.m12, t.m11) * 180.0 / 3.14159265358979323846;
}

} // namespace

class TstToolsInteractivity : public QObject
{
    Q_OBJECT

private slots:
    // Le test qui distingue : la carotte refusée revient par `drop(nullptr)`, sans étape, sans `edit.undo`,
    // sans commande corrective après le dépôt ; elle est à sa place de départ.
    void bounceBackDropsWithoutAStep()
    {
        Rig r;
        r.basket();
        const core::RectF start = r.bounds(kCarrot);
        r.drag(70, 220, 400, 200, 4);
        QCOMPARE(r.editor.calls, (std::vector<std::string>{"lift", "drop:null"}));
        QCOMPARE(r.editor.executes, 0);
        QVERIFY(r.editor.dropped.empty());
        QVERIFY(r.commands.triggered.isEmpty());
        const core::RectF now = r.bounds(kCarrot);
        QVERIFY(closeTo(now.x, start.x) && closeTo(now.y, start.y));
        QVERIFY(r.accepted.empty());
        QVERIFY(!r.tool.isBusy());
    }

    // La pomme acceptée, aimantée au centre du panier : une seule commande au dépôt, déjà à sa place
    // d'accrochage (jamais une seconde commande qui la recentre).
    void acceptedDropIsOneStep()
    {
        Rig r;
        r.basket();
        r.page().objects[1].common.restrictions.magnet = true; // la pomme ; repère : centre
        r.drag(70, 70, 396, 204, 4);                           // centre à 5,7 px de celui du panier
        QCOMPARE(r.editor.calls, (std::vector<std::string>{"lift", "drop:Transform"}));
        QCOMPARE(r.editor.executes, 0);
        QCOMPARE(r.editor.dropped.size(), std::size_t(1));
        QVERIFY(closePoint(r.centre(kApple), 400, 200));
    }

    // Déplacement vertical : le glisser en diagonale ne change pas x, ni pendant le geste ni au dépôt. Chemin
    // : l'objet saute sur son trait et le suit.
    void verticalDragKeepsX()
    {
        {
            Rig r;
            core::PageObject object = rectangle(1, 100, 100, 50, 50);
            object.common.restrictions.travel = core::Travel::Vertical;
            r.page().objects.push_back(std::move(object));
            r.dragTo(125, 125, 225, 205, 4);
            QVERIFY(closeTo(r.lift.last.dx, 0));
            QVERIFY(closeTo(r.lift.last.dy, 80));
            r.release(225, 205);
            QCOMPARE(r.editor.calls, (std::vector<std::string>{"lift", "drop:Transform"}));
            QVERIFY(closePoint(r.centre(1), 125, 205));
        }
        {
            Rig r;
            core::Line line;
            line.points = {{100, 300}, {500, 300}};
            core::PageObject track{line};
            track.common.id = 20;
            r.page().objects.push_back(std::move(track));
            core::PageObject object = rectangle(1, 100, 100, 50, 50);
            object.common.restrictions.travel = core::Travel::AlongTrack;
            object.common.restrictions.track = 20;
            r.page().objects.push_back(std::move(object));
            r.drag(125, 125, 225, 125, 4);
            QVERIFY(closePoint(r.centre(1), 225, 300));
        }
    }

    // Pendant le glisser, au-dessus d'un panier et près d'un bloquant : la page affichée ne reçoit rien
    // (aucune commande, aucun second soulèvement, aucune remise), la couche soulevée n'est montrée qu'une
    // fois et ne reçoit que sa transformation.
    void noCacheRebuildDuringDrag()
    {
        Rig r;
        r.basket();
        core::PageObject wall = rectangle(30, 600, 400, 20, 100);
        wall.common.restrictions.blocksOthers = true;
        r.page().objects.push_back(std::move(wall));
        const std::size_t before = r.page().objects.size();
        r.dragTo(70, 220, 400, 200, 6);
        QCOMPARE(r.editor.calls, (std::vector<std::string>{"lift"}));
        QCOMPARE(r.editor.executes, 0);
        QCOMPARE(r.editor.drops, 0);
        QCOMPARE(r.page().objects.size(), before - 1); // la carotte soulevée seule
        QCOMPARE(r.lift.shows, 1);
        QCOMPARE(r.lift.transforms, 6);
        r.release(400, 200);
        QCOMPARE(r.editor.calls, (std::vector<std::string>{"lift", "drop:null"}));
    }

    // Dimension bloquée (E6, `sizeHandlesShown`) : aucune poignée de dimension, la barre reste ; le mode
    // conception n'y change rien (le booléen du lot 2 fait foi).
    void fixedSizeShowsNoSizeHandles()
    {
        Rig r;
        FakePolicy policy;
        r.tool.setInteractionPolicy(&policy);
        core::PageObject object = rectangle(1, 100, 100, 200, 100);
        object.common.restrictions.resizable = false;
        r.page().objects.push_back(std::move(object));
        r.user.setSelection({1});
        r.tool.refresh();
        QVERIFY(r.selection.last().handles.empty());
        QVERIFY(!r.selection.last().bar.empty());
        policy.restrictions = false;
        r.tool.refresh();
        QVERIFY(r.selection.last().handles.empty());
    }

    // Pas de 45° : le bouton de rotation tiré à 50° pose 45°, pendant le geste et au dépôt, autour du pivot.
    void rotationFollowsTheStep()
    {
        Rig r;
        core::PageObject object = rectangle(1, 100, 100, 200, 100);
        object.common.restrictions.turnStep = 45;
        r.page().objects.push_back(std::move(object));
        r.user.setSelection({1});
        r.tool.refresh();
        const core::PointF centre{200, 150};
        const core::PointF button = r.barSpot(QStringLiteral("object.rotate"));
        QVERIFY(button.x > -1e8);
        const auto at = [&](double degrees) { return core::rotationAbout(centre, degrees).map(button); };
        r.press(button.x, button.y);
        r.move(at(20).x, at(20).y);
        r.move(at(50).x, at(50).y);
        QVERIFY2(closeTo(angleOf(r.lift.last), 45, 1e-6), qPrintable(QString::number(angleOf(r.lift.last))));
        const core::PointF pivot = r.lift.last.map(centre);
        QVERIFY(closePoint(pivot, 200, 150));
        r.release(at(50).x, at(50).y);
        QCOMPARE(r.editor.calls, (std::vector<std::string>{"lift", "drop:Transform"}));
        QVERIFY(closeTo(angleOf(r.editor.find(1)->common.transform), 45, 1e-6));

        // Revue L8-T16, point 4 : sens horaire et pas de 45°, départ à 30°, pivot en haut à gauche
        // (décentré). Le pas porte sur l'angle de l'objet ; un retour en arrière n'est pas suivi, la reprise
        // l'est.
        Rig t;
        core::PageObject turned = rectangle(1, 100, 100, 200, 100);
        turned.common.transform = core::rotationAbout({200, 150}, 30);
        turned.common.restrictions.turnStep = 45;
        turned.common.restrictions.turning = core::Turning::Clockwise;
        turned.common.restrictions.pivot = core::Mark::TopLeft;
        t.page().objects.push_back(std::move(turned));
        const core::PointF pivotPoint = core::pivotOf(t.page(), t.page().objects.front());
        QVERIFY(!closePoint(pivotPoint, 200, 150));
        t.user.setSelection({1});
        t.tool.refresh();
        const core::PointF grip = t.barSpot(QStringLiteral("object.rotate"));
        QVERIFY(grip.x > -1e8);
        const auto around = [&](double degrees) {
            return core::rotationAbout(pivotPoint, degrees).map(grip);
        };
        const auto turnedBy = [&]() { return angleOf(t.lift.last); };
        t.press(grip.x, grip.y);
        t.move(around(20).x, around(20).y); // 50° voulus : 45°
        QVERIFY2(closeTo(turnedBy(), 15, 1e-6), qPrintable(QString::number(turnedBy())));
        t.move(around(50).x, around(50).y); // 80° : 90°
        QVERIFY2(closeTo(turnedBy(), 60, 1e-6), qPrintable(QString::number(turnedBy())));
        t.move(around(20).x, around(20).y); // retour à 50° : refusé, reste à 90°
        QVERIFY2(closeTo(turnedBy(), 60, 1e-6), qPrintable(QString::number(turnedBy())));
        t.move(around(-10).x, around(-10).y); // 20° : refusé
        QVERIFY2(closeTo(turnedBy(), 60, 1e-6), qPrintable(QString::number(turnedBy())));
        t.move(around(80).x, around(80).y);   // 110° : 90°
        t.move(around(100).x, around(100).y); // 130° : 135°
        QVERIFY2(closeTo(turnedBy(), 105, 1e-6), qPrintable(QString::number(turnedBy())));
        QVERIFY(closePoint(t.lift.last.map(pivotPoint), pivotPoint.x, pivotPoint.y)); // le pivot ne bouge pas
        t.release(around(100).x, around(100).y);
        QCOMPARE(t.editor.calls, (std::vector<std::string>{"lift", "drop:Transform"}));
        QVERIFY2(closeTo(angleOf(t.editor.find(1)->common.transform), 135, 1e-6),
                 qPrintable(QString::number(angleOf(t.editor.find(1)->common.transform))));
    }

    // L8-C6 (revue de L8-T15) : le pas de rotation porte sur l'angle de l'objet (core::objectAngle), celui de
    // la propriété Rotation, pas sur atan2(m12, m11). Un objet retourné gauche-droite a l'angle 0 (atan2 :
    // 180) : tiré à 50° avec un pas de 40°, il tourne de 0 à 40°. Rouge sous l'angle brut : 180 + 50
    // s'arrondit à 240, la poignée pose 60°.
    void rotationStepIgnoresTheMirrorHalfTurn()
    {
        Rig r;
        core::PageObject object = rectangle(1, 100, 100, 200, 100);
        object.common.transform = core::scalingAbout({200, 150}, -1, 1);
        object.common.restrictions.turnStep = 40;
        r.page().objects.push_back(std::move(object));
        QVERIFY(closeTo(core::objectAngle(r.page().objects.front().common.transform), 0, 1e-4));
        r.user.setSelection({1});
        r.tool.refresh();
        const core::PointF centre{200, 150};
        const core::PointF button = r.barSpot(QStringLiteral("object.rotate"));
        QVERIFY(button.x > -1e8);
        const auto at = [&](double degrees) { return core::rotationAbout(centre, degrees).map(button); };
        r.press(button.x, button.y);
        r.move(at(20).x, at(20).y);
        r.move(at(50).x, at(50).y);
        QVERIFY2(closeTo(angleOf(r.lift.last), 40, 1e-4), qPrintable(QString::number(angleOf(r.lift.last))));
        r.release(at(50).x, at(50).y);
        QCOMPARE(r.editor.calls, (std::vector<std::string>{"lift", "drop:Transform"}));
        const double dropped = core::objectAngle(r.editor.find(1)->common.transform);
        QVERIFY2(closeTo(dropped, 40, 1e-4), qPrintable(QString::number(dropped)));
    }

    // L8-C6 (revue de L8-T16) : sur un rail courbe, un bloquant raccourcit le déplacement, mais l'objet reste
    // sur son rail. Rail en équerre (100, 300) → (500, 300) → (500, 600) ; l'objet part de (100, 300) vers
    // (500, 500), un mur en travers de la corde, loin du rail. Rouge sous l'arrêt en ligne droite : l'objet
    // s'arrête sur la corde, à y = 377,5, hors du rail.
    void blockerStopsOnTheRailNotOnTheChord()
    {
        Rig r;
        core::Line line;
        line.points = {{100, 300}, {500, 300}, {500, 600}};
        core::PageObject track{line};
        track.common.id = 20;
        r.page().objects.push_back(std::move(track));
        core::PageObject object = rectangle(1, 75, 275, 50, 50); // centre (100, 300), sur le rail
        object.common.restrictions.travel = core::Travel::AlongTrack;
        object.common.restrictions.track = 20;
        r.page().objects.push_back(std::move(object));
        core::PageObject wall = rectangle(30, 280, 390, 40, 40);
        wall.common.restrictions.blocksOthers = true;
        r.page().objects.push_back(std::move(wall));
        r.drag(100, 300, 500, 500, 8);
        QCOMPARE(r.editor.calls, (std::vector<std::string>{"lift", "drop:Transform"}));
        const core::PointF c = r.centre(1);
        QVERIFY2(closeTo(c.y, 300, 1e-4),
                 qPrintable(QStringLiteral("hors du rail : (%1, %2)").arg(c.x).arg(c.y)));
        QVERIFY2(c.x > 100 && c.x < 400, qPrintable(QStringLiteral("arrêté avant le mur : x = %1").arg(c.x)));
    }

    // L8-C6 (revue de L8-T16) : l'objet à chemin saute sur son trait à l'appui (plan L8-T16), pas au premier
    // déplacement de 2 px. Appui puis relâché sans bouger, hors du trait : posé sur le trait, en une étape.
    // Déjà sur son trait : ni soulevé ni déposé, le clic le sélectionne.
    void jumpsOntoItsTrackAtPress()
    {
        const auto railed = [](Rig& r, double top) {
            core::Line line;
            line.points = {{100, 300}, {500, 300}};
            core::PageObject track{line};
            track.common.id = 20;
            r.page().objects.push_back(std::move(track));
            core::PageObject object = rectangle(1, 100, top, 50, 50);
            object.common.restrictions.travel = core::Travel::AlongTrack;
            object.common.restrictions.track = 20;
            r.page().objects.push_back(std::move(object));
        };
        {
            Rig r;
            railed(r, 100); // centre (125, 125), trait à y = 300
            r.press(125, 125);
            r.release(125, 125);
            QCOMPARE(r.editor.calls, (std::vector<std::string>{"lift", "drop:Transform"}));
            QVERIFY(closePoint(r.centre(1), 125, 300));
        }
        {
            Rig r;
            railed(r, 275); // centre (125, 300) : déjà sur son trait
            r.press(125, 300);
            r.release(125, 300);
            QVERIFY(r.editor.calls.empty());
            QCOMPARE(r.user.selection(), (std::vector<core::ObjectId>{1}));
            QVERIFY(closePoint(r.centre(1), 125, 300));
        }
    }

    // Mode conception, restrictions désactivées : la carotte reste dans le panier (une étape, sans signal),
    // le déplacement vertical ne retient plus x.
    void designModeLiftsRestrictions()
    {
        {
            Rig r;
            FakePolicy policy;
            policy.restrictions = false;
            r.tool.setInteractionPolicy(&policy);
            r.basket();
            r.drag(70, 220, 400, 200, 4);
            QCOMPARE(r.editor.calls, (std::vector<std::string>{"lift", "drop:Transform"}));
            QCOMPARE(r.editor.dropped.size(), std::size_t(1));
            QVERIFY(closePoint(r.centre(kCarrot), 400, 200));
            QVERIFY(r.accepted.empty());
        }
        {
            Rig r;
            FakePolicy policy;
            policy.restrictions = false;
            r.tool.setInteractionPolicy(&policy);
            core::PageObject object = rectangle(1, 100, 100, 50, 50);
            object.common.restrictions.travel = core::Travel::Vertical;
            r.page().objects.push_back(std::move(object));
            r.drag(125, 125, 225, 205, 4);
            QVERIFY(closePoint(r.centre(1), 225, 205));
        }
    }

    // Un objet bloquant arrête le glisser au contact, pendant le geste et au dépôt ; objets bloquants
    // désactivés (mode conception) : il passe.
    void blockerStopsTheDrag()
    {
        const auto rig = [](Rig& r) {
            r.page().objects.push_back(rectangle(1, 100, 100, 50, 50));
            core::PageObject wall = rectangle(30, 300, 80, 20, 100);
            wall.common.restrictions.blocksOthers = true;
            r.page().objects.push_back(std::move(wall));
        };
        {
            Rig r;
            rig(r);
            r.dragTo(125, 125, 425, 125, 6);
            QVERIFY(r.lift.last.dx <= 150 + 1e-6);
            r.release(425, 125);
            QVERIFY2(closeTo(r.bounds(1).x, 250), qPrintable(QString::number(r.bounds(1).x)));
        }
        {
            Rig r;
            FakePolicy policy;
            policy.blockers = false;
            r.tool.setInteractionPolicy(&policy);
            rig(r);
            r.drag(125, 125, 425, 125, 6);
            QVERIFY(closeTo(r.bounds(1).x, 400));
        }
    }

    // Pomme acceptée : le rappel donne le panier et son son ; la carotte renvoyée n'en donne aucun.
    void containerSignalEmitted()
    {
        Rig r;
        r.basket();
        r.drag(70, 220, 400, 200, 4);
        QVERIFY(r.accepted.empty());
        r.drag(70, 70, 400, 200, 4);
        QCOMPARE(r.accepted.size(), std::size_t(1));
        QCOMPARE(r.accepted.front().container, kBasket);
        QVERIFY(r.accepted.front().chime);
    }

    // Le test qui distingue (revue L8-T16, point 1) : en multi-sélection, un objet à rotation réglée
    // (bloquée, à sens, à pas ou à pivot décentré) retire le bouton de rotation de la barre, comme un objet
    // non pivotable ; tiré à l'endroit où il serait, rien ne tourne. Mode conception : la sélection tourne.
    void multiSelectionRefusesRuledRotation()
    {
        const auto fixed = [](core::Restrictions& r) { r.turning = core::Turning::Fixed; };
        const auto clockwise = [](core::Restrictions& r) { r.turning = core::Turning::Clockwise; };
        const auto stepped = [](core::Restrictions& r) { r.turnStep = 45; };
        const auto pivoted = [](core::Restrictions& r) { r.pivot = core::Mark::TopLeft; };
        const std::vector<void (*)(core::Restrictions&)> rules{fixed, clockwise, stepped, pivoted};
        for (std::size_t k = 0; k < rules.size(); ++k) {
            Rig r;
            FakePolicy policy;
            r.tool.setInteractionPolicy(&policy);
            core::PageObject ruled = rectangle(1, 100, 100, 100, 100);
            rules[k](ruled.common.restrictions);
            r.page().objects.push_back(std::move(ruled));
            r.page().objects.push_back(rectangle(2, 300, 100, 100, 100));
            const core::PointF centre{250, 150}; // centre du cadre des deux
            const auto turnBy = [&](core::PointF button, double degrees) {
                const auto at = [&](double d) { return core::rotationAbout(centre, d).map(button); };
                r.press(button.x, button.y);
                r.move(at(degrees / 2).x, at(degrees / 2).y);
                r.move(at(degrees).x, at(degrees).y);
                r.release(at(degrees).x, at(degrees).y);
            };
            policy.restrictions = false;
            r.user.setSelection({1, 2});
            r.tool.refresh();
            const core::PointF spot = r.barSpot(QStringLiteral("object.rotate"));
            QVERIFY2(spot.x > -1e8, qPrintable(QString::number(k)));
            policy.restrictions = true;
            r.tool.refresh();
            QVERIFY2(r.barSpot(QStringLiteral("object.rotate")).x < -1e8, qPrintable(QString::number(k)));
            QVERIFY(r.barSpot(QStringLiteral("object.enlarge")).x > -1e8); // la barre reste
            turnBy(spot, 30);
            QVERIFY2(closeTo(angleOf(r.editor.find(1)->common.transform), 0, 1e-6),
                     qPrintable(QString::number(k)));
            QVERIFY2(closeTo(angleOf(r.editor.find(2)->common.transform), 0, 1e-6),
                     qPrintable(QString::number(k)));
            policy.restrictions = false;
            r.user.setSelection({1, 2});
            r.tool.refresh();
            turnBy(spot, 30);
            QVERIFY2(closeTo(angleOf(r.editor.find(1)->common.transform), 30, 1e-6),
                     qPrintable(QString::number(k)));
            QVERIFY(closeTo(angleOf(r.editor.find(2)->common.transform), 30, 1e-6));
        }
    }

    // Revue L8-T16, point 2 : la page d'essai n'est construite qu'au relâché, une fois par geste à règle ;
    // jamais pendant les mouvements, jamais sans règle.
    void probePageBuiltOnlyAtRelease()
    {
        Rig r;
        r.basket();
        core::PageObject wall = rectangle(30, 600, 400, 20, 100);
        wall.common.restrictions.blocksOthers = true;
        r.page().objects.push_back(std::move(wall));
        r.dragTo(70, 220, 400, 200, 6);
        QCOMPARE(r.tool.probePagesBuilt(), std::size_t(0));
        r.release(400, 200);
        QCOMPARE(r.tool.probePagesBuilt(), std::size_t(1));
        QCOMPARE(r.editor.calls, (std::vector<std::string>{"lift", "drop:null"})); // le retour reste exact
        r.dragTo(70, 70, 400, 200, 6);
        QCOMPARE(r.tool.probePagesBuilt(), std::size_t(1));
        r.release(400, 200);
        QCOMPARE(r.tool.probePagesBuilt(), std::size_t(2));
        QCOMPARE(r.accepted.size(), std::size_t(1));

        Rig plain; // sans règle : aucune page d'essai
        plain.page().objects.push_back(rectangle(1, 100, 100, 50, 50));
        plain.drag(125, 125, 225, 205, 4);
        QCOMPARE(plain.tool.probePagesBuilt(), std::size_t(0));
        QVERIFY(closePoint(plain.centre(1), 225, 205));
    }

    // Revue L8-T16, point 3 : l'aimantation passe par les limiteurs et les bloquants, et un dépôt accepté
    // reste dans son conteneur, même si le point d'aimantation (Mark::Point) est ailleurs.
    void magnetObeysRulesAndContainer()
    {
        {
            Rig r; // aimant hors du panier : la pomme acceptée reste dans le panier
            r.basket();
            core::Restrictions& apple = r.page().objects[1].common.restrictions;
            apple.magnet = true;
            apple.magnetMark = core::Mark::Point;
            apple.magnetPoint = {700, 500};
            r.drag(70, 70, 400, 200, 4);
            const core::PointF c = r.centre(kApple);
            const core::RectF basket = r.bounds(kBasket);
            QVERIFY2(c.x > basket.x && c.x < basket.x + basket.width && c.y > basket.y &&
                         c.y < basket.y + basket.height,
                     qPrintable(QString::number(c.x) + QLatin1Char(',') + QString::number(c.y)));
            QCOMPARE(r.editor.dropped.size(), std::size_t(1));
            QCOMPARE(r.accepted.size(), std::size_t(1));
            QCOMPARE(r.accepted.front().container, kBasket);
        }
        {
            Rig r; // aimant dans la tolérance du panier (CentresMatch) : la pomme s'y pose
            r.basket();
            core::Restrictions& apple = r.page().objects[1].common.restrictions;
            apple.magnet = true;
            apple.magnetMark = core::Mark::Point;
            apple.magnetPoint = {403, 197};
            r.drag(70, 70, 396, 204, 4);
            const core::PointF c = r.centre(kApple);
            QVERIFY2(closePoint(c, 403, 197),
                     qPrintable(QString::number(c.x) + QLatin1Char(',') + QString::number(c.y)));
            QCOMPARE(r.accepted.size(), std::size_t(1));
        }
        {
            Rig r; // déplacement horizontal : l'aimant ne change pas y
            core::PageObject object = rectangle(1, 50, 50, 40, 40);
            object.common.restrictions.travel = core::Travel::Horizontal;
            object.common.restrictions.magnet = true;
            object.common.restrictions.magnetMark = core::Mark::Point;
            object.common.restrictions.magnetPoint = {400, 300};
            r.page().objects.push_back(std::move(object));
            r.drag(70, 70, 200, 70, 4);
            const core::PointF c = r.centre(1);
            QVERIFY2(closePoint(c, 400, 70),
                     qPrintable(QString::number(c.x) + QLatin1Char(',') + QString::number(c.y)));
        }
        {
            Rig r; // un mur entre le dépôt et l'aimant : l'objet s'arrête au contact
            core::PageObject object = rectangle(1, 50, 50, 40, 40);
            object.common.restrictions.magnet = true;
            object.common.restrictions.magnetMark = core::Mark::Point;
            object.common.restrictions.magnetPoint = {500, 70};
            r.page().objects.push_back(std::move(object));
            core::PageObject wall = rectangle(30, 300, 0, 20, 200);
            wall.common.restrictions.blocksOthers = true;
            r.page().objects.push_back(std::move(wall));
            r.drag(70, 70, 200, 70, 4);
            QVERIFY2(closeTo(r.bounds(1).x, 260), qPrintable(QString::number(r.bounds(1).x)));
        }
    }

    // Revue L8-T16, point 6 : les flèches du clavier ne déplacent pas la sélection (SelectTool::onKey ne
    // connaît qu'Échap et Suppr) ; elles ne contournent donc ni limiteur ni bloquant. Garde-fou si elles
    // viennent un jour : un objet fixe ou vertical ne bouge pas de travers.
    void arrowKeysDoNotBypassRules()
    {
        Rig r;
        core::PageObject still = rectangle(1, 100, 100, 50, 50);
        still.common.restrictions.travel = core::Travel::Fixed;
        r.page().objects.push_back(std::move(still));
        core::PageObject upright = rectangle(2, 300, 100, 50, 50);
        upright.common.restrictions.travel = core::Travel::Vertical;
        r.page().objects.push_back(std::move(upright));
        r.user.setSelection({1, 2});
        r.tool.refresh();
        for (const int key : {int(Qt::Key_Right), int(Qt::Key_Left), int(Qt::Key_Up), int(Qt::Key_Down)}) {
            r.tool.onKey(key, 0);
            r.tool.onKey(key, int(Qt::ShiftModifier));
        }
        QVERIFY(closePoint(r.centre(1), 125, 125));
        QVERIFY(closeTo(r.centre(2).x, 325));
        QCOMPARE(r.editor.executes, 0);
    }
};

QTEST_GUILESS_MAIN(TstToolsInteractivity)
#include "tst_tools_interactivity.moc"
