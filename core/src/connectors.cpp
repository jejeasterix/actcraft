#include <actcraft/core/connectors.h>
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/objectedits.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <utility>

namespace actcraft::core {

namespace {

constexpr std::size_t kAbsent = std::numeric_limits<std::size_t>::max();

std::vector<PointF> corners(const RectF& r)
{
    return {{r.x, r.y}, {r.x + r.width, r.y}, {r.x + r.width, r.y + r.height}, {r.x, r.y + r.height}};
}

// Sommets d'une forme, en coordonnées de l'objet ; vide si l'objet n'est pas une forme.
std::vector<PointF> shapeVertices(const Shape& shape)
{
    const RectF& b = shape.bounds;
    switch (shape.kind) {
    case ShapeKind::Rectangle:
        return corners(b);
    case ShapeKind::Ellipse: // sommets de l'ellipse : extrémités de ses axes
        return {{b.x + b.width / 2, b.y},
                {b.x + b.width, b.y + b.height / 2},
                {b.x + b.width / 2, b.y + b.height},
                {b.x, b.y + b.height / 2}};
    case ShapeKind::Polygon:
        return shape.polygon.empty() ? corners(b) : shape.polygon;
    case ShapeKind::Path:
        return corners(b);
    }
    return corners(b);
}

std::size_t find(const Page& page, ObjectId id) noexcept
{
    if (id == 0) {
        return kAbsent;
    }
    const std::size_t i = indexOf(page, id);
    return i < page.objects.size() && page.objects[i].common.id == id ? i : kAbsent;
}

double distance(PointF a, PointF b) noexcept
{
    return std::hypot(a.x - b.x, a.y - b.y);
}

std::size_t nearestIndex(const std::vector<PointF>& points, PointF p) noexcept
{
    std::size_t best = 0;
    for (std::size_t i = 1; i < points.size(); ++i) {
        if (distance(points[i], p) < distance(points[best], p)) {
            best = i;
        }
    }
    return best;
}

bool samePoint(PointF a, PointF b) noexcept
{
    return a.x == b.x && a.y == b.y;
}

bool samePoints(const std::vector<PointF>& a, const std::vector<PointF>& b) noexcept
{
    return std::equal(a.begin(), a.end(), b.begin(), b.end(), samePoint);
}

bool sameTransform(const Transform& a, const Transform& b) noexcept
{
    return a.m11 == b.m11 && a.m12 == b.m12 && a.m21 == b.m21 && a.m22 == b.m22 && a.dx == b.dx &&
           a.dy == b.dy;
}

// Bout lié, relevé avant la commande : l'objet, l'indice de son point d'attache le plus proche du bout et ses
// points d'attache d'alors (pour savoir s'il a bougé).
struct LinkedEnd
{
    ObjectId object = 0;
    std::size_t index = 0;
    std::vector<PointF> points;
};
// Connecteur relié, relevé avant la commande : sa position d'alors (pour savoir s'il a bougé) et ses bouts.
struct Link
{
    ObjectId connector = 0;
    Transform transform;
    PointF firstEnd;
    PointF secondEnd;
    std::optional<LinkedEnd> first;
    std::optional<LinkedEnd> second;
};

std::optional<LinkedEnd> linkedEnd(const Page& page, ObjectId object, PointF endOnPage)
{
    const std::size_t i = find(page, object);
    if (i == kAbsent) {
        return std::nullopt;
    }
    auto points = attachPoints(page.objects[i]);
    if (points.empty()) {
        return std::nullopt;
    }
    const std::size_t index = nearestIndex(points, endOnPage);
    return LinkedEnd{object, index, std::move(points)};
}

std::vector<Link> linksOf(const Page& page)
{
    std::vector<Link> links;
    for (const PageObject& object : page.objects) {
        const auto* c = std::get_if<Connector>(&object);
        if (!c || (c->objectA == 0 && c->objectB == 0)) {
            continue;
        }
        const Transform& t = object.common.transform;
        Link link{object.common.id,
                  t,
                  c->firstEnd,
                  c->secondEnd,
                  linkedEnd(page, c->objectA, t.map(c->firstEnd)),
                  linkedEnd(page, c->objectB, t.map(c->secondEnd))};
        if (link.first || link.second) {
            links.push_back(std::move(link));
        }
    }
    return links;
}

// Un bout suit si son objet a bougé ou disparu, ou si le connecteur a bougé (soulevé avec son objet, par
// exemple) ; une commande sans rapport laisse un bout où il est, même posé à côté d'un sommet.
bool mustFollow(const Page& page, const LinkedEnd& link, bool connectorMoved)
{
    if (connectorMoved) {
        return true;
    }
    const std::size_t i = find(page, link.object);
    return i == kAbsent || !samePoints(attachPoints(page.objects[i]), link.points);
}

// Page insérée, retirée ou remplacée (Rétablir la page) : la commande donne la page telle qu'elle la veut.
// Un PageChanged d'une autre commande (Effacer, grille, fond) ne remplace pas la page : les bouts suivent ou
// se détachent comme pour une suppression.
bool reshapesPages(const Changes& changes, std::size_t from, bool replaces) noexcept
{
    return std::any_of(
        changes.begin() + static_cast<std::ptrdiff_t>(from), changes.end(), [replaces](const Change& c) {
            return c.kind == Change::Kind::PageInserted || c.kind == Change::Kind::PageRemoved ||
                   (replaces && c.kind == Change::Kind::PageChanged);
        });
}

// Après la commande : le bout rejoint le même point d'attache de son objet, ou se détache sur place.
void follow(const Page& page, const Transform& connectorTransform, const LinkedEnd& link, ObjectId& object,
            PointF& end)
{
    const std::size_t i = find(page, link.object);
    if (i == kAbsent) {
        object = 0; // l'objet n'est plus là : le bout reste où il est
        return;
    }
    const auto points = attachPoints(page.objects[i]);
    const auto toLocal = inverted(connectorTransform);
    if (points.empty() || !toLocal) {
        return;
    }
    const std::size_t index =
        link.index < points.size() ? link.index : nearestIndex(points, connectorTransform.map(end));
    end = toLocal->map(points[index]);
    object = link.object;
}

} // namespace

std::vector<PointF> attachPoints(const PageObject& object)
{
    if (std::get_if<Group>(&object)) {
        return {}; // un groupe n'a pas d'emprise propre : on s'attache à ses membres
    }
    if (const auto* shape = std::get_if<Shape>(&object)) {
        auto points = shapeVertices(*shape);
        for (auto& p : points) {
            p = object.common.transform.map(p);
        }
        return points;
    }
    return corners(boundsOnPage(object));
}

std::optional<Attachment> snapConnectorEnd(const Page& page, PointF point)
{
    std::optional<Attachment> best;
    double bestDistance = kConnectorSnap; // « à moins de 20 » (D11) : 20 tout juste n'accroche pas
    for (auto it = page.objects.rbegin(); it != page.objects.rend(); ++it) { // du dessus vers le dessous
        if (std::get_if<Connector>(&*it)) {
            continue;
        }
        for (const PointF& p : attachPoints(*it)) {
            const double d = distance(p, point);
            if (d < bestDistance) {
                bestDistance = d;
                best = Attachment{it->common.id, p};
            }
        }
    }
    return best;
}

namespace {

constexpr ConnectorStyle kStyles[kConnectorStyleCount] = {
    {ConnectorRoute::Straight, false, false, false}, {ConnectorRoute::Straight, true, false, false},
    {ConnectorRoute::Straight, false, true, false},  {ConnectorRoute::Straight, true, true, false},
    {ConnectorRoute::Straight, false, false, true},  {ConnectorRoute::Straight, true, false, true},
    {ConnectorRoute::Straight, false, true, true},   {ConnectorRoute::Straight, true, true, true},
    {ConnectorRoute::CurveA, false, false, false},   {ConnectorRoute::CurveB, false, false, false},
    {ConnectorRoute::Step, false, false, false},     {ConnectorRoute::Elbow, false, false, false},
    {ConnectorRoute::Angle, false, false, false},    {ConnectorRoute::Angle, true, false, false},
};

// Bout de la polyligne de contrôle : le premier point distinct de `tip` en partant de lui donne la tangente.
ConnectorTip tipOf(const std::vector<PointF>& points, bool atEnd)
{
    const std::size_t n = points.size();
    ConnectorTip tip;
    tip.point = atEnd ? points[n - 1] : points[0];
    for (std::size_t i = 1; i < n; ++i) {
        const PointF& inner = atEnd ? points[n - 1 - i] : points[i];
        const double length = distance(tip.point, inner);
        if (length > 0) {
            tip.outward = {(tip.point.x - inner.x) / length, (tip.point.y - inner.y) / length};
            tip.reach = length;
            break;
        }
    }
    return tip;
}

} // namespace

ConnectorStyle connectorStyle(int rank) noexcept
{
    return rank >= 0 && rank < kConnectorStyleCount ? kStyles[rank] : kStyles[0];
}

std::vector<PointF> connectorControlPoints(const Connector& c)
{
    const PointF a = c.firstEnd;
    const PointF b = c.secondEnd;
    const double midX = a.x + 0.5 * (b.x - a.x);
    switch (connectorStyle(c.style).route) {
    case ConnectorRoute::CurveA:
        return {a, {a.x, b.y}, b};
    case ConnectorRoute::CurveB:
        return {a, {b.x, a.y}, b};
    case ConnectorRoute::Step:
        return {a, {midX, a.y}, {midX, b.y}, b};
    case ConnectorRoute::Elbow:
        return {a, {b.x, a.y}, b};
    case ConnectorRoute::Angle:
        return {a, {midX, b.y}, b};
    case ConnectorRoute::Straight:
        break;
    }
    return {a, b};
}

std::vector<PointF> connectorPath(const Connector& c)
{
    std::vector<PointF> control = connectorControlPoints(c);
    const ConnectorRoute route = connectorStyle(c.style).route;
    if (route != ConnectorRoute::CurveA && route != ConnectorRoute::CurveB) {
        return control;
    }
    const PointF a = control[0];
    const PointF k = control[1];
    const PointF b = control[2];
    std::vector<PointF> curve;
    curve.reserve(kConnectorCurveSegments + 1);
    for (int i = 0; i <= kConnectorCurveSegments; ++i) {
        const double t = static_cast<double>(i) / kConnectorCurveSegments;
        const double u = 1 - t;
        curve.push_back(
            {u * u * a.x + 2 * u * t * k.x + t * t * b.x, u * u * a.y + 2 * u * t * k.y + t * t * b.y});
    }
    return curve;
}

ConnectorTips connectorTips(const Connector& c)
{
    const std::vector<PointF> control = connectorControlPoints(c);
    return {tipOf(control, false), tipOf(control, true)};
}

PageObject makeConnector(PointF a, ObjectId objectA, PointF b, ObjectId objectB, std::uint8_t style)
{
    Connector c;
    c.firstEnd = a;
    c.secondEnd = b;
    c.objectA = objectA;
    c.objectB = objectB;
    c.style = style;
    PageObject object = c;
    object.common.layer = Layer::Top;
    return object;
}

FollowingConnectorsCommand::FollowingConnectorsCommand(std::size_t page, std::unique_ptr<UndoCommand> inner)
    : m_page(page), m_inner(std::move(inner))
{}

void FollowingConnectorsCommand::redo(Document& document, Changes& changes)
{
    if (!m_inner) {
        return;
    }
    m_connectorsBefore.clear();
    const std::vector<Link> links = m_page < document.pageCount() ? linksOf(document.page(m_page))
                                                                  : std::vector<Link>{}; // avant la commande
    const std::size_t from = changes.size();
    m_inner->redo(document, changes);
    if (links.empty() || m_page >= document.pageCount() ||
        reshapesPages(changes, from, dynamic_cast<const ReplacePageCommand*>(m_inner.get()) != nullptr)) {
        return;
    }
    Page& page = document.page(m_page);
    for (const Link& link : links) {
        const std::size_t i = find(page, link.connector);
        if (i == kAbsent || !std::get_if<Connector>(&page.objects[i])) {
            continue; // retiré par la commande
        }
        PageObject& object = page.objects[i];
        PageObject before = object; // après la commande, avant le suivi : ce que l'annulation remet
        auto& c = std::get<Connector>(object);
        const bool connectorMoved = !sameTransform(object.common.transform, link.transform) ||
                                    !samePoint(c.firstEnd, link.firstEnd) ||
                                    !samePoint(c.secondEnd, link.secondEnd);
        if (link.first && mustFollow(page, *link.first, connectorMoved)) {
            follow(page, object.common.transform, *link.first, c.objectA, c.firstEnd);
        }
        if (link.second && mustFollow(page, *link.second, connectorMoved)) {
            follow(page, object.common.transform, *link.second, c.objectB, c.secondEnd);
        }
        const auto& old = std::get<Connector>(before);
        if (samePoint(old.firstEnd, c.firstEnd) && samePoint(old.secondEnd, c.secondEnd) &&
            old.objectA == c.objectA && old.objectB == c.objectB) {
            continue;
        }
        changes.push_back({Change::Kind::ObjectChanged, m_page, link.connector,
                           enclosingRect(boundsOnPage(before), boundsOnPage(object))});
        m_connectorsBefore.push_back(std::move(before));
    }
}

void FollowingConnectorsCommand::undo(Document& document, Changes& changes)
{
    if (!m_inner) {
        return;
    }
    // Inverse exact du redo : connecteurs remis à leur état d'après la commande, puis commande annulée. Sans
    // connecteur suivi, la page n'est pas lue : la commande a pu la retirer.
    if (!m_connectorsBefore.empty()) {
        Page& page = document.page(m_page);
        for (auto it = m_connectorsBefore.rbegin(); it != m_connectorsBefore.rend(); ++it) {
            const std::size_t i = find(page, it->common.id);
            if (i == kAbsent) {
                continue;
            }
            const RectF area = enclosingRect(boundsOnPage(page.objects[i]), boundsOnPage(*it));
            page.objects[i] = *it;
            changes.push_back({Change::Kind::ObjectChanged, m_page, it->common.id, area});
        }
    }
    m_inner->undo(document, changes);
}

int FollowingConnectorsCommand::mergeId() const noexcept
{
    return m_inner ? m_inner->mergeId() : -1;
}

// Une réglette (lot 3) garde sa fusion sur une page reliée : la commande suivante, qui n'a suivi aucun
// connecteur, s'absorbe dans la commande enveloppée ; l'annulation remet les connecteurs de la première.
bool FollowingConnectorsCommand::mergeWith(const UndoCommand& other)
{
    if (!m_inner) {
        return false;
    }
    const auto* wrapped = dynamic_cast<const FollowingConnectorsCommand*>(&other);
    if (!wrapped) {
        return m_inner->mergeWith(other);
    }
    return wrapped->m_inner && wrapped->m_page == m_page && wrapped->m_connectorsBefore.empty() &&
           m_inner->mergeWith(*wrapped->m_inner);
}

std::unique_ptr<UndoCommand> withFollowingConnectors(const Page& page, std::size_t pageIndex,
                                                     std::unique_ptr<UndoCommand> inner)
{
    const bool linked = std::any_of(page.objects.begin(), page.objects.end(), [](const PageObject& object) {
        const auto* c = std::get_if<Connector>(&object);
        return c && (c->objectA != 0 || c->objectB != 0);
    });
    if (!inner || !linked) {
        return inner; // rien à suivre : la commande garde sa fusion (glissements du lot 3)
    }
    return std::make_unique<FollowingConnectorsCommand>(pageIndex, std::move(inner));
}

} // namespace actcraft::core
