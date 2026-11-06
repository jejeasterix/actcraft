#include <actcraft/core/edits.h>
#include <actcraft/core/restrictors.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

// Boîtes : boundsOnPage(page, objet) (edits.h), celle de la touche, du cadre de sélection et des zones du
// cache : emprise transformée, épaisseur du contour comprise, groupes résolus par la page.
namespace actcraft::core {

namespace {

const PageObject* findOnPage(const Page& page, ObjectId id) noexcept
{
    if (id == 0) {
        return nullptr;
    }
    const std::size_t index = indexOf(page, id);
    return index < page.objects.size() ? &page.objects[index] : nullptr;
}

PointF markOf(const RectF& box, Mark mark) noexcept
{
    const double left = box.x;
    const double top = box.y;
    const double right = box.x + box.width;
    const double bottom = box.y + box.height;
    const PointF centre = box.center();
    switch (mark) {
    case Mark::TopLeft:
        return {left, top};
    case Mark::Top:
        return {centre.x, top};
    case Mark::TopRight:
        return {right, top};
    case Mark::Right:
        return {right, centre.y};
    case Mark::BottomRight:
        return {right, bottom};
    case Mark::Bottom:
        return {centre.x, bottom};
    case Mark::BottomLeft:
        return {left, bottom};
    case Mark::Left:
        return {left, centre.y};
    case Mark::Center:
    case Mark::FirstTextLine: // non tranché (D8 de L8-T0) : centre
    case Mark::Point:
    case Mark::OtherObject:
        break;
    }
    return centre;
}

// Chemin en coordonnées de page : points du trait ou de la ligne passés par sa transformation.
std::vector<PointF> trackOnPage(const PageObject& track)
{
    std::vector<PointF> points;
    const Transform& t = track.common.transform;
    if (const auto* stroke = std::get_if<Stroke>(&track)) {
        points.reserve(stroke->points.size());
        for (const StrokePoint& p : stroke->points) {
            points.push_back(t.map({p.x, p.y}));
        }
    } else if (const auto* line = std::get_if<Line>(&track)) {
        points.reserve(line->points.size());
        for (const PointF& p : line->points) {
            points.push_back(t.map(p));
        }
    }
    return points;
}

PointF closestOnSegment(PointF p, PointF a, PointF b) noexcept
{
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double length2 = dx * dx + dy * dy;
    if (!(length2 > 0)) {
        return a;
    }
    const double t = std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / length2, 0.0, 1.0);
    return {a.x + t * dx, a.y + t * dy};
}

double distance2(PointF a, PointF b) noexcept
{
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return dx * dx + dy * dy;
}

// Fraction t ∈ [0, 1] du déplacement `wanted` à laquelle `box` touche `blocker` en entrant dedans ; 1 s'il
// ne le rencontre pas. Recouvrement strict : bord contre bord est permis, glisser le long d'un bord aussi.
// Une boîte qui recouvre déjà le bloquant au départ n'est pas retenue par lui (elle peut en sortir).
double firstContact(const RectF& box, const RectF& blocker, PointF wanted) noexcept
{
    double enter = -std::numeric_limits<double>::infinity();
    double exit = std::numeric_limits<double>::infinity();
    const auto axis = [&](double low, double size, double blockerLow, double blockerSize, double d) {
        const double minGap = blockerLow - (low + size);      // recouvrement si t·d > minGap
        const double maxGap = blockerLow + blockerSize - low; // … et t·d < maxGap
        if (d == 0) {
            return minGap < 0 && maxGap > 0;
        }
        double t1 = minGap / d;
        double t2 = maxGap / d;
        if (t1 > t2) {
            std::swap(t1, t2);
        }
        enter = std::max(enter, t1);
        exit = std::min(exit, t2);
        return true;
    };
    if (!axis(box.x, box.width, blocker.x, blocker.width, wanted.x) ||
        !axis(box.y, box.height, blocker.y, blocker.height, wanted.y)) {
        return 1;
    }
    if (!(enter < exit) || enter < 0 || enter >= 1) {
        return 1;
    }
    return enter;
}

bool contains(const std::vector<ObjectId>& ids, ObjectId id) noexcept
{
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

} // namespace

PointF allowedTravel(const Page& page, const PageObject& object, PointF wanted)
{
    const Restrictions& r = object.common.restrictions;
    if (!r.movable) {
        return {}; // le booléen du lot 2 fait foi, quel que soit le mode (objet bloqué lu d'un fichier)
    }
    switch (r.travel) {
    case Travel::Free:
        return wanted;
    case Travel::Vertical:
        return {0, wanted.y};
    case Travel::Horizontal:
        return {wanted.x, 0};
    case Travel::AlongTrack: {
        const PointF start = boundsOnPage(page, object).center();
        const PointF landed = projectOnTrack(page, object, {start.x + wanted.x, start.y + wanted.y});
        return {landed.x - start.x, landed.y - start.y};
    }
    case Travel::Fixed:
        break;
    }
    return {};
}

PointF projectOnTrack(const Page& page, const PageObject& object, PointF point)
{
    const Restrictions& r = object.common.restrictions;
    if (!r.movable || r.travel != Travel::AlongTrack || r.track == object.common.id) {
        return point;
    }
    const PageObject* track = findOnPage(page, r.track);
    if (track == nullptr) {
        return point;
    }
    const std::vector<PointF> path = trackOnPage(*track);
    if (path.empty()) {
        return point;
    }
    PointF best = path.front();
    double bestDistance = distance2(point, best);
    for (std::size_t i = 1; i < path.size(); ++i) {
        const PointF candidate = closestOnSegment(point, path[i - 1], path[i]);
        const double d = distance2(point, candidate);
        if (d < bestDistance) {
            best = candidate;
            bestDistance = d;
        }
    }
    return best;
}

PointF landingPoint(const Page& page, const PageObject& object, PointF dropped)
{
    const Restrictions& r = object.common.restrictions;
    if (!r.magnet) {
        return dropped;
    }
    if (r.magnetMark == Mark::Point) {
        return r.magnetPoint; // point précis, coordonnées de page : le centre s'y pose
    }
    // Dernier conteneur (le plus haut dans la superposition) sous le point de dépôt.
    const PageObject* container = nullptr;
    for (auto it = page.objects.rbegin(); it != page.objects.rend(); ++it) {
        if (it->common.id == object.common.id || it->common.container.accepts == Accepts::Nothing ||
            !hasExtent(page, *it)) {
            continue;
        }
        if (boundsOnPage(page, *it).contains(dropped)) {
            container = &*it;
            break;
        }
    }
    if (container == nullptr) {
        return dropped;
    }
    // Le repère de l'objet, lâché centré sur `dropped`, se pose sur le même repère du conteneur.
    const RectF own = boundsOnPage(page, object);
    const RectF atDrop{dropped.x - own.width / 2, dropped.y - own.height / 2, own.width, own.height};
    const PointF ownMark = markOf(atDrop, r.magnetMark);
    const PointF target = markOf(boundsOnPage(page, *container), r.magnetMark);
    return {dropped.x + target.x - ownMark.x, dropped.y + target.y - ownMark.y};
}

bool sizeHandlesShown(const PageObject& object) noexcept
{
    return object.common.restrictions.resizable; // E6 (D6 de L8-T0) : « bloqué », aucune poignée
}

double allowedTurn(const PageObject& object, double fromDegrees, double wantedDegrees) noexcept
{
    const Restrictions& r = object.common.restrictions;
    if (!r.rotatable || r.turning == Turning::Fixed) {
        return fromDegrees; // le booléen du lot 2 fait foi, quel que soit le mode
    }
    double angle = wantedDegrees;
    if (r.turnStep > 0 && std::isfinite(r.turnStep)) {
        angle = std::round(wantedDegrees / r.turnStep) * r.turnStep; // pas : au plus proche
    }
    // Sens du tour le plus court de `from` à `angle` : positif = horaire (rotation(), model.h).
    const double turn = std::remainder(angle - fromDegrees, 360.0);
    if ((r.turning == Turning::Clockwise && turn < 0) ||
        (r.turning == Turning::CounterClockwise && turn > 0)) {
        return fromDegrees;
    }
    return angle;
}

PointF pivotOf(const Page& page, const PageObject& object)
{
    const Restrictions& r = object.common.restrictions;
    const RectF box = boundsOnPage(page, object);
    if (r.pivot == Mark::Point) {
        return object.common.transform.map(r.pivotPoint); // coordonnées de l'objet → page
    }
    if (r.pivot == Mark::OtherObject) {
        const PageObject* other = findOnPage(page, r.pivotObject);
        if (other != nullptr && other->common.id != object.common.id && hasExtent(page, *other)) {
            return boundsOnPage(page, *other).center();
        }
        return box.center();
    }
    return markOf(box, r.pivot);
}

PointF stopAtBlockers(const Page& page, const std::vector<ObjectId>& moving, PointF wanted)
{
    bool any = false;
    double left = 0, top = 0, right = 0, bottom = 0;
    for (const PageObject& o : page.objects) {
        if (!contains(moving, o.common.id) || !hasExtent(page, o)) {
            continue;
        }
        const RectF b = boundsOnPage(page, o);
        left = any ? std::min(left, b.x) : b.x;
        top = any ? std::min(top, b.y) : b.y;
        right = any ? std::max(right, b.x + b.width) : b.x + b.width;
        bottom = any ? std::max(bottom, b.y + b.height) : b.y + b.height;
        any = true;
    }
    if (!any) {
        return wanted;
    }
    const RectF box{left, top, right - left, bottom - top};
    double t = 1;
    for (const PageObject& o : page.objects) {
        if (!o.common.restrictions.blocksOthers || contains(moving, o.common.id) || !hasExtent(page, o)) {
            continue;
        }
        t = std::min(t, firstContact(box, boundsOnPage(page, o), wanted));
    }
    return {wanted.x * t, wanted.y * t};
}

} // namespace actcraft::core
