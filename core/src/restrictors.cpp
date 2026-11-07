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

// Point de la polyligne (points de l'objet passés par sa transformation : coordonnées de page) le plus proche
// de `point`, segment par segment ; aucune allocation (appelée à chaque mouvement). `points` non vide.
template <typename Points, typename ToPoint>
PointF nearestOnPolyline(const Points& points, const Transform& t, PointF point, ToPoint toPoint) noexcept
{
    PointF previous = t.map(toPoint(points.front()));
    PointF best = previous;
    double bestDistance = distance2(point, best);
    for (std::size_t i = 1; i < points.size(); ++i) {
        const PointF current = t.map(toPoint(points[i]));
        const PointF candidate = closestOnSegment(point, previous, current);
        const double d = distance2(point, candidate);
        if (d < bestDistance) {
            best = candidate;
            bestDistance = d;
        }
        previous = current;
    }
    return best;
}

// Recouvrement plus petit : contact. La position t·wanted d'un arrêt bord contre bord recouvre parfois le
// bloquant d'un ulp ; lu comme un recouvrement, il laisserait le glisser suivant traverser.
constexpr double kContact = 1e-6;

// Instants t (fraction de `wanted`) où `box` recouvre `blocker` réduit de `margin` sur chaque bord :
// intervalle ouvert ]enter, exit[, vide si enter >= exit.
std::pair<double, double> overlapTimes(const RectF& box, const RectF& blocker, PointF wanted,
                                       double margin) noexcept
{
    double enter = -std::numeric_limits<double>::infinity();
    double exit = std::numeric_limits<double>::infinity();
    const auto axis = [&](double low, double size, double blockerLow, double blockerSize, double d) {
        const double minGap = blockerLow + margin - (low + size);      // recouvrement si t·d > minGap
        const double maxGap = blockerLow + blockerSize - margin - low; // … et t·d < maxGap
        if (d == 0) {
            if (!(minGap < 0 && maxGap > 0)) {
                enter = std::numeric_limits<double>::infinity(); // jamais
                exit = -std::numeric_limits<double>::infinity();
            }
            return;
        }
        double t1 = minGap / d;
        double t2 = maxGap / d;
        if (t1 > t2) {
            std::swap(t1, t2);
        }
        enter = std::max(enter, t1);
        exit = std::min(exit, t2);
    };
    axis(box.x, box.width, blocker.x, blocker.width, wanted.x);
    axis(box.y, box.height, blocker.y, blocker.height, wanted.y);
    return {enter, exit};
}

// Vrai si le déplacement fait décroître la pénétration de `box` dans `blocker` (axe le moins enfoncé).
bool movingAway(const RectF& box, const RectF& blocker, PointF wanted) noexcept
{
    const auto depth = [](double low, double size, double blockerLow, double blockerSize,
                          double d) -> std::pair<double, double> {
        const double fromLow = low + size - blockerLow;         // croît avec d
        const double fromHigh = blockerLow + blockerSize - low; // décroît avec d
        if (fromLow < fromHigh) {
            return {fromLow, d};
        }
        if (fromHigh < fromLow) {
            return {fromHigh, -d};
        }
        return {fromLow, -std::abs(d)};
    };
    const auto [depthX, rateX] = depth(box.x, box.width, blocker.x, blocker.width, wanted.x);
    const auto [depthY, rateY] = depth(box.y, box.height, blocker.y, blocker.height, wanted.y);
    const double rate = depthX < depthY ? rateX : (depthY < depthX ? rateY : std::min(rateX, rateY));
    return rate < 0;
}

// Fraction t ∈ [0, 1] du déplacement `wanted` que `blocker` laisse à `box` ; 1 s'il ne l'arrête pas. Bord
// contre bord et glisser le long d'un bord sont permis ; l'arrêt se fait au contact exact. Une boîte qui
// recouvre vraiment le bloquant au départ (au-delà de kContact) n'est libre que si elle s'en éloigne.
double firstContact(const RectF& box, const RectF& blocker, PointF wanted) noexcept
{
    const auto [enter, exit] = overlapTimes(box, blocker, wanted, kContact);
    if (!(enter < exit) || exit <= 0 || enter >= 1) {
        return 1; // ne s'enfonce pas au-delà du contact pendant ce déplacement
    }
    if (enter < 0) {
        return movingAway(box, blocker, wanted) ? 1 : 0;
    }
    const double touch = overlapTimes(box, blocker, wanted, 0).first;
    return std::clamp(touch, 0.0, enter);
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
    const Transform& t = track->common.transform;
    if (const auto* stroke = std::get_if<Stroke>(track); stroke && !stroke->points.empty()) {
        return nearestOnPolyline(stroke->points, t, point,
                                 [](const StrokePoint& p) { return PointF{p.x, p.y}; });
    }
    if (const auto* line = std::get_if<Line>(track); line && !line->points.empty()) {
        return nearestOnPolyline(line->points, t, point, [](const PointF& p) { return p; });
    }
    return point;
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
    // Dernier conteneur (le plus haut dans la superposition) sous le point de dépôt, sans règle d'acceptation
    // (containers.h, L8-T14) : le choix du conteneur revient à l'appelant (L8-T16), qui n'accroche que si ce
    // conteneur accepte l'objet.
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
    // Tour le plus court du dernier angle permis à l'angle voulu : positif = horaire (rotation(), model.h).
    // Sûr tant que deux mouvements successifs sont à moins d'un demi-tour l'un de l'autre.
    const double turn = std::remainder(angle - fromDegrees, 360.0);
    if ((r.turning == Turning::Clockwise && turn < 0) ||
        (r.turning == Turning::CounterClockwise && turn > 0)) {
        return fromDegrees;
    }
    return fromDegrees + turn; // continu avec le précédent, même si l'angle voulu saute de 360°
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
    const auto isBlocker = [&](const PageObject& o) {
        return o.common.restrictions.blocksOthers && !contains(moving, o.common.id) && hasExtent(page, o);
    };
    if (std::none_of(page.objects.begin(), page.objects.end(), isBlocker)) {
        return wanted; // cas courant : aucun bloquant hors des objets soulevés, rien à calculer
    }
    // Une boîte par objet soulevé : leur union recouvrirait un bloquant placé entre eux.
    std::vector<RectF> boxes;
    boxes.reserve(moving.size());
    std::vector<RectF> blockers;
    for (const PageObject& o : page.objects) {
        if (contains(moving, o.common.id) && hasExtent(page, o)) {
            boxes.push_back(boundsOnPage(page, o));
        } else if (isBlocker(o)) {
            blockers.push_back(boundsOnPage(page, o));
        }
    }
    return stopBoxesAtBlockers(boxes, blockers, wanted);
}

PointF stopBoxesAtBlockers(const std::vector<RectF>& moving, const std::vector<RectF>& blockers,
                           PointF wanted) noexcept
{
    double t = 1;
    for (const RectF& blocker : blockers) {
        for (const RectF& box : moving) {
            t = std::min(t, firstContact(box, blocker, wanted));
        }
    }
    return {wanted.x * t, wanted.y * t};
}

} // namespace actcraft::core
