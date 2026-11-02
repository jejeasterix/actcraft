#include <actcraft/core/connectors.h>
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>

#include <algorithm>
#include <cmath>
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

std::optional<Transform> inverted(const Transform& t) noexcept
{
    const double det = t.m11 * t.m22 - t.m12 * t.m21;
    if (det == 0 || !std::isfinite(det)) {
        return std::nullopt;
    }
    Transform r;
    r.m11 = t.m22 / det;
    r.m12 = -t.m12 / det;
    r.m21 = -t.m21 / det;
    r.m22 = t.m11 / det;
    r.dx = -(r.m11 * t.dx + r.m21 * t.dy);
    r.dy = -(r.m12 * t.dx + r.m22 * t.dy);
    return r;
}

RectF unite(const RectF& a, const RectF& b) noexcept
{
    const double left = std::min(a.x, b.x);
    const double top = std::min(a.y, b.y);
    const double right = std::max(a.x + a.width, b.x + b.width);
    const double bottom = std::max(a.y + a.height, b.y + b.height);
    return {left, top, right - left, bottom - top};
}

bool samePoint(PointF a, PointF b) noexcept
{
    return a.x == b.x && a.y == b.y;
}

// Bout lié, relevé avant la commande : l'objet et l'indice de son point d'attache le plus proche du bout.
struct LinkedEnd
{
    ObjectId object = 0;
    std::size_t index = 0;
};
struct Link
{
    ObjectId connector = 0;
    std::optional<LinkedEnd> first;
    std::optional<LinkedEnd> second;
};

std::optional<LinkedEnd> linkedEnd(const Page& page, ObjectId object, PointF endOnPage)
{
    const std::size_t i = find(page, object);
    if (i == kAbsent) {
        return std::nullopt;
    }
    const auto points = attachPoints(page.objects[i]);
    if (points.empty()) {
        return std::nullopt;
    }
    return LinkedEnd{object, nearestIndex(points, endOnPage)};
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
        Link link{object.common.id, linkedEnd(page, c->objectA, t.map(c->firstEnd)),
                  linkedEnd(page, c->objectB, t.map(c->secondEnd))};
        if (link.first || link.second) {
            links.push_back(std::move(link));
        }
    }
    return links;
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
    const std::vector<Link> links = linksOf(document.page(m_page)); // avant la commande
    m_inner->redo(document, changes);
    m_connectorsBefore.clear();
    Page& page = document.page(m_page);
    for (const Link& link : links) {
        const std::size_t i = find(page, link.connector);
        if (i == kAbsent || !std::get_if<Connector>(&page.objects[i])) {
            continue; // retiré par la commande
        }
        PageObject& object = page.objects[i];
        PageObject before = object; // après la commande, avant le suivi : ce que l'annulation remet
        auto& c = std::get<Connector>(object);
        if (link.first) {
            follow(page, object.common.transform, *link.first, c.objectA, c.firstEnd);
        }
        if (link.second) {
            follow(page, object.common.transform, *link.second, c.objectB, c.secondEnd);
        }
        const auto& old = std::get<Connector>(before);
        if (samePoint(old.firstEnd, c.firstEnd) && samePoint(old.secondEnd, c.secondEnd) &&
            old.objectA == c.objectA && old.objectB == c.objectB) {
            continue;
        }
        changes.push_back({Change::Kind::ObjectChanged, m_page, link.connector,
                           unite(boundsOnPage(before), boundsOnPage(object))});
        m_connectorsBefore.push_back(std::move(before));
    }
}

void FollowingConnectorsCommand::undo(Document& document, Changes& changes)
{
    if (!m_inner) {
        return;
    }
    // Inverse exact du redo : connecteurs remis à leur état d'après la commande, puis commande annulée.
    Page& page = document.page(m_page);
    for (auto it = m_connectorsBefore.rbegin(); it != m_connectorsBefore.rend(); ++it) {
        const std::size_t i = find(page, it->common.id);
        if (i == kAbsent) {
            continue;
        }
        const RectF area = unite(boundsOnPage(page.objects[i]), boundsOnPage(*it));
        page.objects[i] = *it;
        changes.push_back({Change::Kind::ObjectChanged, m_page, it->common.id, area});
    }
    m_inner->undo(document, changes);
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
