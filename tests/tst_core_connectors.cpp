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

namespace {

// L5-C3 : comparaison de points à 1e-9 près (les directions sont normées, les courbes échantillonnées).
bool same(core::PointF p, double x, double y)
{
    return std::abs(p.x - x) < 1e-9 && std::abs(p.y - y) < 1e-9;
}

std::vector<XY> xys(const std::vector<core::PointF>& points)
{
    std::vector<XY> out;
    for (const core::PointF& p : points) {
        out.push_back(xy(p));
    }
    return out;
}

core::Connector styled(int rank, core::PointF a, core::PointF b)
{
    core::Connector c;
    c.firstEnd = a;
    c.secondEnd = b;
    c.style = static_cast<std::uint8_t>(rank);
    return c;
}

bool inside(const core::RectF& box, core::PointF p)
{
    constexpr double e = 1e-9;
    return p.x >= box.x - e && p.x <= box.x + box.width + e && p.y >= box.y - e &&
           p.y <= box.y + box.height + e;
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

    // L5-C23 (mineur C) : Effacer ▸ Objets retire l'objet relié ; le connecteur verrouillé qui reste détache
    // son bout (comme la suppression), et l'annulation le rattache.
    void clearingObjectsDetachesTheEnd()
    {
        auto d = blank();
        const auto a = addRect(d, {100, 100, 100, 50});
        const auto c = addConnectorAt(d, {200, 150}, a, {600, 600}, 0);
        objectById(d, c).common.locked = true; // survit à l'effacement
        core::UndoStack stack;
        stack.push(d,
                   core::withFollowingConnectors(
                       d.page(0), 0, std::make_unique<core::ClearPageCommand>(0, core::ClearKind::Objects)));
        QCOMPARE(core::indexOf(d.page(0), a), d.page(0).objects.size()); // l'objet est parti
        QCOMPARE(connector(d, c).objectA, core::ObjectId(0));
        QCOMPARE(xy(connector(d, c).firstEnd), XY(200, 150)); // reste en place
        stack.undo(d);
        QCOMPARE(connector(d, c).objectA, a);
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

    // L5-C3 : les 14 rangs du panneau, (tracé, flèche au départ, flèche à l'arrivée, pointillés), d'après
    // notre ancêtre Python (CONNECTOR_TYPES) ; un rang hors du panneau est la ligne simple du rang 0.
    void styleTableFollowsThePanel()
    {
        using R = core::ConnectorRoute;
        struct Row
        {
            R route;
            bool start;
            bool end;
            bool dashed;
        };
        const Row rows[] = {
            {R::Straight, false, false, false}, {R::Straight, true, false, false},
            {R::Straight, false, true, false},  {R::Straight, true, true, false},
            {R::Straight, false, false, true},  {R::Straight, true, false, true},
            {R::Straight, false, true, true},   {R::Straight, true, true, true},
            {R::CurveA, false, false, false},   {R::CurveB, false, false, false},
            {R::Step, false, false, false},     {R::Elbow, false, false, false},
            {R::Angle, false, false, false},    {R::Angle, true, false, false},
        };
        QCOMPARE(core::kConnectorStyleCount, 14);
        for (int rank = 0; rank < core::kConnectorStyleCount; ++rank) {
            const core::ConnectorStyle s = core::connectorStyle(rank);
            QCOMPARE(s.route, rows[rank].route);
            QCOMPARE(s.arrowStart, rows[rank].start);
            QCOMPARE(s.arrowEnd, rows[rank].end);
            QCOMPARE(s.dashed, rows[rank].dashed);
        }
        for (int outside : {14, 255, -1}) {
            const core::ConnectorStyle s = core::connectorStyle(outside);
            QCOMPARE(s.route, R::Straight);
            QVERIFY(!s.arrowStart && !s.arrowEnd && !s.dashed);
        }
    }

    // L5-C3 : polygone de contrôle de chaque tracé, a (20, 30), b (220, 130) ; la courbe A part verticale
    // (contrôle (a.x, b.y)), la B horizontale (contrôle (b.x, a.y)).
    void controlPointsFollowTheRoute()
    {
        const core::PointF a{20, 30};
        const core::PointF b{220, 130};
        const auto points = [&](int rank) { return xys(core::connectorControlPoints(styled(rank, a, b))); };
        QCOMPARE(points(0), (std::vector<XY>{{20, 30}, {220, 130}}));
        QCOMPARE(points(6), (std::vector<XY>{{20, 30}, {220, 130}}));
        QCOMPARE(points(8), (std::vector<XY>{{20, 30}, {20, 130}, {220, 130}}));
        QCOMPARE(points(9), (std::vector<XY>{{20, 30}, {220, 30}, {220, 130}}));
        QCOMPARE(points(10), (std::vector<XY>{{20, 30}, {120, 30}, {120, 130}, {220, 130}}));
        QCOMPARE(points(11), (std::vector<XY>{{20, 30}, {220, 30}, {220, 130}}));
        QCOMPARE(points(12), (std::vector<XY>{{20, 30}, {120, 130}, {220, 130}}));
        QCOMPARE(points(13), (std::vector<XY>{{20, 30}, {120, 130}, {220, 130}}));
    }

    // L5-C3 : le tracé dessiné est la polyligne de contrôle, la courbe échantillonnée en
    // kConnectorCurveSegments segments égaux en paramètre.
    void pathSamplesTheCurve()
    {
        const core::PointF a{20, 30};
        const core::PointF b{220, 130};
        QCOMPARE(xys(core::connectorPath(styled(11, a, b))),
                 xys(core::connectorControlPoints(styled(11, a, b))));
        const std::vector<core::PointF> curve = core::connectorPath(styled(8, a, b));
        QCOMPARE(int(curve.size()), core::kConnectorCurveSegments + 1);
        QVERIFY(same(curve.front(), 20, 30));
        QVERIFY(same(curve.back(), 220, 130));
        // B(1/2) = a/4 + c/2 + b/4, c = (20, 130)
        QVERIFY(same(curve[core::kConnectorCurveSegments / 2], 70, 105));
        const std::vector<core::PointF> curveB = core::connectorPath(styled(9, a, b));
        QVERIFY(same(curveB[core::kConnectorCurveSegments / 2], 170, 55)); // c = (220, 30)
    }

    // L5-C3 : la flèche suit la tangente du tracé à son bout (premier segment de contrôle non nul), pas la
    // droite a→b ; `outward` pointe hors du tracé, `reach` borne la flèche à ce segment.
    void tipsFollowTheEndTangent()
    {
        const core::PointF a{20, 30};
        const core::PointF b{220, 130};
        const double n = std::hypot(200.0, 100.0);
        const auto tips = [&](int rank) { return core::connectorTips(styled(rank, a, b)); };

        const core::ConnectorTips line = tips(2);
        QVERIFY(same(line.a.point, 20, 30) && same(line.b.point, 220, 130));
        QVERIFY(same(line.b.outward, 200 / n, 100 / n));
        QVERIFY(same(line.a.outward, -200 / n, -100 / n));
        QCOMPARE(line.b.reach, n);

        const core::ConnectorTips curveA = tips(8);
        QVERIFY(same(curveA.a.outward, 0, -1)); // part verticale
        QVERIFY(same(curveA.b.outward, 1, 0));  // arrive horizontale
        QCOMPARE(curveA.a.reach, 100.0);
        QCOMPARE(curveA.b.reach, 200.0);

        const core::ConnectorTips curveB = tips(9);
        QVERIFY(same(curveB.a.outward, -1, 0));
        QVERIFY(same(curveB.b.outward, 0, 1));

        const core::ConnectorTips step = tips(10);
        QVERIFY(same(step.a.outward, -1, 0));
        QVERIFY(same(step.b.outward, 1, 0));
        QCOMPARE(step.b.reach, 100.0);

        const core::ConnectorTips elbow = tips(11);
        QVERIFY(same(elbow.a.outward, -1, 0));
        QVERIFY(same(elbow.b.outward, 0, 1));

        const core::ConnectorTips angle = tips(13);
        QVERIFY(same(angle.a.outward, -std::sqrt(0.5), -std::sqrt(0.5)));
        QVERIFY(same(angle.b.outward, 1, 0));
        QCOMPARE(angle.b.reach, 100.0);
    }

    // L5-C3 : un segment de contrôle nul est sauté (marches ou courbe à a.x == b.x) ; a == b n'a pas de sens.
    void degenerateRoutesKeepATangent()
    {
        const core::ConnectorTips step = core::connectorTips(styled(10, {50, 10}, {50, 200}));
        QVERIFY(same(step.a.outward, 0, -1));
        QVERIFY(same(step.b.outward, 0, 1));
        const core::ConnectorTips curve = core::connectorTips(styled(8, {50, 10}, {50, 200}));
        QVERIFY(same(curve.b.outward, 0, 1));
        const core::ConnectorTips point = core::connectorTips(styled(12, {50, 50}, {50, 50}));
        QVERIFY(same(point.a.outward, 0, 0) && same(point.b.outward, 0, 0));
        QCOMPARE(point.b.reach, 0.0);
    }

    // L5-C3 : tous les tracés restent dans le rectangle a–b et chaque flèche (le long de sa tangente, bornée
    // par `reach`, demi-largeur connectorCapSize / 2) dans la marge : l'emprise et la touche d'avant
    // suffisent.
    void routesAndArrowsStayInTheExtent()
    {
        const std::pair<core::PointF, core::PointF> ends[] = {
            {{20, 30}, {220, 130}}, {{220, 130}, {20, 30}}, {{50, 10}, {50, 200}},
            {{0, 0}, {2, 100}},     {{0, 0}, {100, 2}},     {{300, 40}, {10, 400}},
        };
        for (int rank = 0; rank < core::kConnectorStyleCount; ++rank) {
            for (const auto& [a, b] : ends) {
                core::Connector c = styled(rank, a, b);
                c.pen.width = 2;
                c.capA = core::ConnectorCap::Arrow;
                c.capB = core::ConnectorCap::Arrow;
                const core::PageObject object = c;
                const core::RectF box = core::boundsOnPage(object);
                for (const core::PointF& p : core::connectorPath(c)) {
                    QVERIFY2(inside(box, p), qPrintable(QStringLiteral("rang %1").arg(rank)));
                }
                const double size = core::connectorCapSize(c.pen.width);
                const core::ConnectorTips tips = core::connectorTips(c);
                for (const core::ConnectorTip& tip : {tips.a, tips.b}) {
                    const double length = std::min(size, tip.reach);
                    const core::PointF base{tip.point.x - tip.outward.x * length,
                                            tip.point.y - tip.outward.y * length};
                    const core::PointF side{-tip.outward.y * size / 2, tip.outward.x * size / 2};
                    QVERIFY2(inside(box, {base.x + side.x, base.y + side.y}),
                             qPrintable(QStringLiteral("rang %1").arg(rank)));
                    QVERIFY2(inside(box, {base.x - side.x, base.y - side.y}),
                             qPrintable(QStringLiteral("rang %1").arg(rank)));
                }
            }
        }
    }
};

QTEST_GUILESS_MAIN(TestConnectors)
#include "tst_core_connectors.moc"
