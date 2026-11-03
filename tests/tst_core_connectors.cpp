#include <actcraft/core/alignment.h>
#include <actcraft/core/arrange.h>
#include <actcraft/core/clipboardedits.h>
#include <actcraft/core/connectors.h>
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/core/undo.h>

#include <QTest>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace core = actcraft::core;

namespace {

using XY = std::pair<double, double>;

XY xy(core::PointF p)
{
    return {p.x, p.y};
}

core::Document blank()
{
    return core::Document(); // une page de 1024 × 768
}

core::ObjectId addRect(core::Document& d, core::RectF bounds)
{
    core::Shape s;
    s.bounds = bounds;
    s.outlineWidth = 0;
    core::PageObject o = s;
    o.common.id = d.allocateId();
    d.page(0).objects.push_back(std::move(o));
    return d.page(0).objects.back().common.id;
}

core::ObjectId addConnectorAt(core::Document& d, core::PointF pa, core::ObjectId a, core::PointF pb,
                              core::ObjectId b)
{
    core::PageObject o = core::makeConnector(pa, a, pb, b, 0);
    o.common.id = d.allocateId();
    d.page(0).objects.push_back(std::move(o));
    return d.page(0).objects.back().common.id;
}

core::PageObject& objectById(core::Document& d, core::ObjectId id)
{
    auto& page = d.page(0);
    return page.objects[core::indexOf(page, id)];
}

const core::Connector& connector(core::Document& d, core::ObjectId id)
{
    return std::get<core::Connector>(objectById(d, id));
}

// Extrémités en coordonnées de page (la transformation du connecteur appliquée).
XY firstOnPage(core::Document& d, core::ObjectId id)
{
    const auto& o = objectById(d, id);
    return xy(o.common.transform.map(std::get<core::Connector>(o).firstEnd));
}
XY secondOnPage(core::Document& d, core::ObjectId id)
{
    const auto& o = objectById(d, id);
    return xy(o.common.transform.map(std::get<core::Connector>(o).secondEnd));
}

bool contains(const std::vector<core::PointF>& points, core::PointF p)
{
    return std::any_of(points.begin(), points.end(), [p](core::PointF q) {
        return std::abs(q.x - p.x) < 1e-9 && std::abs(q.y - p.y) < 1e-9;
    });
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

// TransformObjectsCommand (A7) : un seul delta pour tous les objets.
std::unique_ptr<core::UndoCommand> translateCommand(std::vector<core::ObjectId> ids, double dx, double dy)
{
    return std::make_unique<core::TransformObjectsCommand>(0, std::move(ids), core::translation(dx, dy));
}

int changedCount(const core::Changes& changes, core::ObjectId id)
{
    return static_cast<int>(std::count_if(changes.begin(), changes.end(), [id](const core::Change& ch) {
        return ch.kind == core::Change::Kind::ObjectChanged && ch.object == id;
    }));
}

} // namespace

class TestConnectors : public QObject
{
    Q_OBJECT
private slots:
    void attachPointsAreShapeVertices()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 100, 50});
        const auto points = core::attachPoints(objectById(d, a));
        QCOMPARE(points.size(), std::size_t(4));
        QVERIFY(contains(points, {200, 150}));
        QVERIFY(contains(points, {100, 100}));
        // Coordonnées de page : la transformation de l'objet s'applique.
        objectById(d, a).common.transform = core::translation(10, 0);
        QVERIFY(contains(core::attachPoints(objectById(d, a)), {210, 150}));
        // Polygone : ses sommets.
        core::Shape tri;
        tri.kind = core::ShapeKind::Polygon;
        tri.polygon = {{0, 0}, {40, 0}, {20, 30}};
        tri.bounds = {0, 0, 40, 30};
        const core::PageObject triangle = tri;
        const auto vertices = core::attachPoints(triangle);
        QCOMPARE(vertices.size(), std::size_t(3));
        QVERIFY(contains(vertices, {20, 30}));
    }

    void endSnapsWithin20()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 100, 50});
        const auto hit = core::snapConnectorEnd(d.page(0), {215, 160}); // à 18 du coin (200, 150)
        QVERIFY(hit.has_value());
        QCOMPARE(hit->object, a);
        QCOMPARE(xy(hit->point), XY(200, 150));
        QVERIFY(!core::snapConnectorEnd(d.page(0), {240, 160}));
        QVERIFY(!core::snapConnectorEnd(d.page(0), {220, 150})); // à 20 tout juste : « à moins de 20 » (D11)
        // Les connecteurs sont exclus : le coin de l'emprise d'un connecteur n'attire rien.
        addConnectorAt(d, {600, 600}, 0, {700, 700}, 0);
        QVERIFY(!core::snapConnectorEnd(d.page(0), {605, 600}));
    }

    void connectorFollowsTransform()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 100, 50});
        const auto b = addRect(d, {400, 100, 100, 50});
        const auto c = addConnectorAt(d, {200, 150}, a, {400, 100}, b);
        core::UndoStack stack;
        stack.push(d, core::withFollowingConnectors(d.page(0), 0, translateCommand({a}, 50, 0)));
        QCOMPARE(stack.count(), std::size_t(1));
        QCOMPARE(xy(connector(d, c).firstEnd), XY(250, 150));
        QCOMPARE(xy(connector(d, c).secondEnd), XY(400, 100));
        QCOMPARE(connector(d, c).objectA, a);
        QCOMPARE(connector(d, c).objectB, b);
    }

    void connectorFollowsAlign()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 100, 50});
        const auto b = addRect(d, {400, 300, 100, 50});
        const auto c = addConnectorAt(d, {200, 150}, a, {400, 300}, b);
        const auto t = core::alignTransforms(d.page(0), {a, b}, core::AlignMode::Top);
        core::UndoStack stack;
        stack.push(d, core::withFollowingConnectors(d.page(0), 0,
                                                    std::make_unique<core::TransformEachCommand>(0, t)));
        QCOMPARE(xy(connector(d, c).secondEnd), XY(400, 100));
        QCOMPARE(xy(connector(d, c).firstEnd), XY(200, 150));
    }

    void undoRestoresConnectorAndObject()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 100, 50});
        const auto c = addConnectorAt(d, {200, 150}, a, {600, 600}, 0);
        const auto before = core::fingerprint(d.page(0));
        core::UndoStack stack;
        stack.push(d, core::withFollowingConnectors(d.page(0), 0, translateCommand({a}, 30, 30)));
        QCOMPARE(xy(connector(d, c).firstEnd), XY(230, 180));
        stack.undo(d);
        QCOMPARE(core::fingerprint(d.page(0)), before);
        stack.redo(d);
        QCOMPARE(xy(connector(d, c).firstEnd), XY(230, 180));
        QCOMPARE(xy(connector(d, c).secondEnd), XY(600, 600));
    }

    void deletingAnEndDetaches()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 100, 50});
        const auto c = addConnectorAt(d, {200, 150}, a, {600, 600}, 0);
        core::UndoStack stack;
        stack.push(d, core::withFollowingConnectors(d.page(0), 0, core::makeCutCommand(d.page(0), 0, {a})));
        QCOMPARE(connector(d, c).objectA, core::ObjectId(0));
        QCOMPARE(xy(connector(d, c).firstEnd), XY(200, 150)); // reste en place
        stack.undo(d);
        QCOMPARE(connector(d, c).objectA, a); // l'annulation le rattache
    }

    // Distingue le suivi par indice de point d'attache (contrat) du suivi par décalage, du réaccrochage par
    // distance et du suivi propre à une commande : après une mise à l'échelle, le coin bas droit reste le
    // coin bas droit.
    void followsTheSameAttachPointWhenScaled()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 100, 50});
        const auto c = addConnectorAt(d, {200, 150}, a, {600, 600}, 0);
        core::TransformList list;
        list.emplace_back(a, scalingAbout(2.0, {100, 100}));
        core::UndoStack stack;
        stack.push(d, core::withFollowingConnectors(d.page(0), 0,
                                                    std::make_unique<core::TransformEachCommand>(0, list)));
        QCOMPARE(connector(d, c).objectA, a);
        QCOMPARE(xy(connector(d, c).firstEnd), XY(300, 200));
        QCOMPARE(xy(connector(d, c).secondEnd), XY(600, 600));
    }

    // Distingue l'ordre de l'annulation : la commande déplace aussi le connecteur, dont un bout reste lié à
    // un objet immobile. Annuler la commande avant de remettre le connecteur laisse le connecteur déplacé.
    void undoRestoresConnectorMovedWithItsObject()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 100, 50});
        const auto b = addRect(d, {400, 100, 100, 50});
        const auto c = addConnectorAt(d, {200, 150}, a, {400, 100}, b);
        const auto before = core::fingerprint(d.page(0));
        core::UndoStack stack;
        stack.push(d, core::withFollowingConnectors(d.page(0), 0, translateCommand({a, c}, 30, 30)));
        QCOMPARE(firstOnPage(d, c), XY(230, 180));
        QCOMPARE(secondOnPage(d, c), XY(400, 100)); // le bout lié à b reste sur b
        stack.undo(d);
        QCOMPARE(core::fingerprint(d.page(0)), before);
        stack.redo(d);
        QCOMPARE(firstOnPage(d, c), XY(230, 180));
        QCOMPARE(secondOnPage(d, c), XY(400, 100));
    }

    // Une étape ; un ObjectChanged par connecteur modifié, aucun pour un connecteur que rien ne bouge.
    void followingIsOneStepWithOneChangePerConnector()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 100, 50});
        const auto b = addRect(d, {400, 100, 100, 50});
        const auto c = addConnectorAt(d, {200, 150}, a, {400, 100}, b);
        const auto still = addConnectorAt(d, {500, 150}, b, {700, 700}, 0); // lié à b, immobile
        core::UndoStack stack;
        const auto changes =
            stack.push(d, core::withFollowingConnectors(d.page(0), 0, translateCommand({a}, 50, 0)));
        QCOMPARE(stack.count(), std::size_t(1));
        QCOMPARE(changedCount(changes, a), 1);
        QCOMPARE(changedCount(changes, c), 1);
        QCOMPARE(changedCount(changes, still), 0);
        const auto& ch = *std::find_if(changes.begin(), changes.end(),
                                       [c](const core::Change& x) { return x.object == c; });
        QVERIFY(ch.area.x <= 200 && ch.area.x + ch.area.width >= 400); // union avant {200..400} et après
        const auto undone = stack.undo(d);
        QCOMPARE(changedCount(undone, c), 1);
        QCOMPARE(changedCount(undone, still), 0);
    }

    void makeConnectorLandsInTheTopLayer()
    {
        const auto o = core::makeConnector({1, 2}, 7, {3, 4}, 0, 5);
        QVERIFY(o.common.layer == core::Layer::Top);
        const auto& c = std::get<core::Connector>(o);
        QCOMPARE(xy(c.firstEnd), XY(1, 2));
        QCOMPARE(xy(c.secondEnd), XY(3, 4));
        QCOMPARE(c.objectA, core::ObjectId(7));
        QCOMPARE(c.objectB, core::ObjectId(0));
        QCOMPARE(c.style, std::uint8_t(5));
    }

    // Grouper (L5-T5) ne déplace rien : aucun bout ne bouge ni ne se détache, aucun ObjectChanged du
    // connecteur.
    void groupingKeepsConnectorsAttached()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 100, 50});
        const auto b = addRect(d, {400, 100, 100, 50});
        const auto c = addConnectorAt(d, {200, 150}, a, {400, 100}, b);
        core::UndoStack stack;
        const auto changes = stack.push(
            d, core::withFollowingConnectors(d.page(0), 0, core::makeGroupCommand(d.page(0), 0, {a, b})));
        QCOMPARE(changedCount(changes, c), 0);
        QCOMPARE(connector(d, c).objectA, a);
        QCOMPARE(connector(d, c).objectB, b);
        QCOMPARE(xy(connector(d, c).firstEnd), XY(200, 150));
        QCOMPARE(xy(connector(d, c).secondEnd), XY(400, 100));
    }

    // Sans connecteur lié sur la page, la commande n'est pas enveloppée : elle garde sa fusion (lot 3).
    void withoutLinkedConnectorTheCommandIsUnwrapped()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 100, 50});
        addConnectorAt(d, {600, 600}, 0, {700, 700}, 0); // libre aux deux bouts
        auto inner = translateCommand({a}, 5, 0);
        const auto* raw = inner.get();
        QCOMPARE(core::withFollowingConnectors(d.page(0), 0, std::move(inner)).get(), raw);
        addConnectorAt(d, {200, 150}, a, {700, 700}, 0);
        auto again = translateCommand({a}, 5, 0);
        const auto* rawAgain = again.get();
        QVERIFY(core::withFollowingConnectors(d.page(0), 0, std::move(again)).get() != rawAgain);
    }

    // L5-C18 : le contrôleur enveloppe toute commande d'une page reliée ; une réglette (fusion, lot 3) y
    // garde une seule étape.
    void wrappedSliderCommandsStillMerge()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 100, 50});
        const auto b = addRect(d, {400, 100, 100, 50});
        addConnectorAt(d, {200, 100}, a, {400, 100}, b);
        core::UndoStack stack;
        for (const std::uint8_t value : std::initializer_list<std::uint8_t>{40, 80, 120}) {
            stack.push(d, core::withFollowingConnectors(d.page(0), 0,
                                                        std::make_unique<core::SetTranslucencyCommand>(
                                                            0, std::vector<core::ObjectId>{a}, value)));
        }
        QCOMPARE(stack.count(), std::size_t(1));
    }

    // L5-C18 : un bout lié ne se recale que si son objet bouge ou disparaît, ou si le connecteur bouge ; une
    // commande sans rapport laisse où il est un bout posé près d'un sommet sans être dessus.
    void unrelatedCommandLeavesEndsAlone()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 100, 50});
        const auto c = addConnectorAt(d, {205, 103}, a, {400, 300}, 0); // près du sommet (200, 100)
        core::Shape other;
        other.bounds = {600, 600, 10, 10};
        core::UndoStack stack;
        stack.push(d,
                   core::withFollowingConnectors(
                       d.page(0), 0, std::make_unique<core::AddObjectCommand>(0, core::PageObject{other})));
        QCOMPARE(d.page(0).objects.size(), std::size_t(3));
        QCOMPARE(xy(connector(d, c).firstEnd), XY(205, 103));
        QCOMPARE(connector(d, c).objectA, a);
    }

    // L5-C18 : « Rétablir la page » rend la page de référence telle quelle ; ses connecteurs ne suivent pas
    // les liens de la page remplacée.
    void revertedPageKeepsItsConnectorsAsSaved()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 100, 50});
        const auto e = addRect(d, {100, 400, 100, 50});
        const auto c = addConnectorAt(d, {200, 100}, a, {700, 700}, 0);
        core::Page reference = d.page(0);
        auto& saved = std::get<core::Connector>(reference.objects[core::indexOf(reference, c)]);
        saved.objectA = e; // dans la référence, le bout est sur l'autre forme
        saved.firstEnd = {200, 400};
        core::UndoStack stack;
        stack.push(d, core::withFollowingConnectors(
                          d.page(0), 0, std::make_unique<core::ReplacePageCommand>(0, std::move(reference))));
        QCOMPARE(connector(d, c).objectA, e);
        QCOMPARE(xy(connector(d, c).firstEnd), XY(200, 400));
    }

    // M8 (L5-C18) : flèche et disque débordent du trait jusqu'à max(3 × largeur, 8) / 2 ; l'emprise (zone
    // redessinée) et la touche les comptent.
    void capsWidenTheExtentAndTheTouch()
    {
        auto d = blank();
        const auto c = addConnectorAt(d, {100, 100}, 0, {300, 100}, 0);
        auto& object = objectById(d, c);
        auto& body = std::get<core::Connector>(object);
        body.pen.width = 2;
        QCOMPARE(core::boundsOnPage(object).y, 99.0); // sans extrémité : le trait et la moitié de sa largeur
        body.capA = core::ConnectorCap::Round;
        body.capB = core::ConnectorCap::Arrow;
        const core::RectF capped = core::boundsOnPage(object);
        QVERIFY(capped.contains({292, 103.5})); // aile de la flèche : base à 292, demi-largeur 4
        QVERIFY(capped.contains({96.5, 100}));  // bord du disque, rayon 4
        QVERIFY(core::objectAt(d.page(0), {292, 103.5}, 0) == c);
        QVERIFY(core::objectAt(d.page(0), {96.5, 100}, 0) == c);
    }
};

QTEST_GUILESS_MAIN(TestConnectors)
#include "tst_core_connectors.moc"
