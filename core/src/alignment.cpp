#include <actcraft/core/alignment.h>
#include <actcraft/core/arrange.h>
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>

#include <algorithm>
#include <cmath>
#include <set>
#include <variant>

namespace actcraft::core {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kEps = 1e-9;

bool sameTransform(const Transform& a, const Transform& b) noexcept
{
    return a.m11 == b.m11 && a.m12 == b.m12 && a.m21 == b.m21 && a.m22 == b.m22 && a.dx == b.dx &&
           a.dy == b.dy;
}

// Unité d'édition : un objet, ou un groupe pris comme un bloc (L5-C17, D6) ; `parts` sont les objets
// réellement transformés (les membres d'un groupe), `bounds` l'emprise de l'unité sur la page.
struct Unit
{
    const PageObject* head = nullptr;
    std::vector<const PageObject*> parts;
    RectF bounds;
    bool group = false;
};

RectF unionRects(const std::vector<const PageObject*>& objects)
{
    double x0 = 0;
    double y0 = 0;
    double x1 = 0;
    double y1 = 0;
    bool first = true;
    for (const PageObject* object : objects) {
        const RectF r = boundsOnPage(*object);
        x0 = first ? r.x : std::min(x0, r.x);
        y0 = first ? r.y : std::min(y0, r.y);
        x1 = first ? r.x + r.width : std::max(x1, r.x + r.width);
        y1 = first ? r.y + r.height : std::max(y1, r.y + r.height);
        first = false;
    }
    return RectF{x0, y0, x1 - x0, y1 - y0};
}

// Unités présentes, sans doublon, déverrouillées et d'emprise non nulle, dans l'ordre des identifiants
// donnés ; un membre cité avec son groupe n'est pas compté deux fois.
std::vector<Unit> units(const Page& page, const std::vector<ObjectId>& ids)
{
    std::set<ObjectId> covered;
    for (const ObjectId id : ids) {
        const std::size_t rank = indexOf(page, id);
        if (rank < page.objects.size() && std::holds_alternative<Group>(body(page.objects[rank]))) {
            for (const ObjectId member : expandGroups(page, {id})) {
                if (member != id) {
                    covered.insert(member);
                }
            }
        }
    }
    std::vector<Unit> out;
    std::set<ObjectId> seen;
    for (const ObjectId id : ids) {
        const std::size_t rank = indexOf(page, id);
        if (rank >= page.objects.size() || covered.count(id) != 0 || !seen.insert(id).second) {
            continue;
        }
        const PageObject& object = page.objects[rank];
        if (object.common.locked) {
            continue;
        }
        Unit unit;
        unit.head = &object;
        if (std::holds_alternative<Group>(body(object))) {
            unit.group = true;
            for (const ObjectId member : expandGroups(page, {id})) {
                const PageObject& m = page.objects[indexOf(page, member)];
                if (!std::holds_alternative<Group>(body(m)) && !m.common.locked && hasExtent(m)) {
                    unit.parts.push_back(&m);
                }
            }
            if (unit.parts.empty()) {
                continue;
            }
            unit.bounds = unionRects(unit.parts);
        } else {
            if (!hasExtent(object)) {
                continue;
            }
            unit.parts = {&object};
            unit.bounds = boundsOnPage(object);
        }
        out.push_back(std::move(unit));
    }
    return out;
}

// Applique `delta` (dans le plan de la page) à chaque objet de l'unité.
void pushUnitDelta(TransformList& out, const Unit& unit, const Transform& delta);

PageObject withTransform(const PageObject& object, const Transform& transform)
{
    PageObject copy = object;
    copy.common.transform = transform;
    return copy;
}

RectF localExtent(const PageObject& object)
{
    return boundsOnPage(withTransform(object, Transform{}));
}

double rotationDegrees(const Transform& t) noexcept
{
    return std::atan2(t.m12, t.m11) * 180.0 / kPi;
}

Transform rotationAbout(double degrees, PointF c) noexcept
{
    const double r = degrees * kPi / 180.0;
    Transform t;
    t.m11 = std::cos(r);
    t.m12 = std::sin(r);
    t.m21 = -std::sin(r);
    t.m22 = std::cos(r);
    t.dx = c.x - (t.m11 * c.x + t.m21 * c.y);
    t.dy = c.y - (t.m12 * c.x + t.m22 * c.y);
    return t;
}

Transform scalingAbout(double sx, double sy, PointF c) noexcept
{
    Transform t;
    t.m11 = sx;
    t.m22 = sy;
    t.dx = c.x - sx * c.x;
    t.dy = c.y - sy * c.y;
    return t;
}

// Réflexion par la droite verticale x = at (horizontal) ou horizontale y = at.
Transform reflection(bool horizontalAxisIsX, double at) noexcept
{
    Transform t;
    if (horizontalAxisIsX) { // Axis::Y : gauche / droite, la droite est verticale
        t.m11 = -1;
        t.dx = 2 * at;
    } else {
        t.m22 = -1;
        t.dy = 2 * at;
    }
    return t;
}

void push(TransformList& out, const PageObject& object, const Transform& absolute)
{
    if (!sameTransform(object.common.transform, absolute)) {
        out.emplace_back(object.common.id, absolute);
    }
}

void pushDelta(TransformList& out, const PageObject& object, const Transform& delta)
{
    push(out, object, compose(object.common.transform, delta));
}

void pushUnitDelta(TransformList& out, const Unit& unit, const Transform& delta)
{
    for (const PageObject* part : unit.parts) {
        pushDelta(out, *part, delta);
    }
}

// Absolu : la taille, l'angle et le centre de `ref` (mode « Tout »).
Transform matchingAll(const PageObject& object, const PageObject& ref)
{
    const RectF lb = localExtent(object);
    const RectF lr = localExtent(ref);
    const double kx = lb.width > kEps && lr.width > kEps ? lr.width / lb.width : 1.0;
    const double ky = lb.height > kEps && lr.height > kEps ? lr.height / lb.height : 1.0;
    Transform t = ref.common.transform;
    t.m11 *= kx;
    t.m12 *= kx;
    t.m21 *= ky;
    t.m22 *= ky;
    const PointF target = ref.common.transform.map(lr.center());
    const PointF c = lb.center();
    t.dx = target.x - (t.m11 * c.x + t.m21 * c.y);
    t.dy = target.y - (t.m12 * c.x + t.m22 * c.y);
    return t;
}

// Décale `r` dans la page, par le plus petit déplacement ; l'emporte le bord gauche / haut s'il est trop
// grand.
PointF shiftInto(const RectF& r, double pageWidth, double pageHeight) noexcept
{
    double dx = 0;
    double dy = 0;
    if (r.x + r.width > pageWidth) {
        dx = pageWidth - (r.x + r.width);
    }
    if (r.x + dx < 0) {
        dx = -r.x;
    }
    if (r.y + r.height > pageHeight) {
        dy = pageHeight - (r.y + r.height);
    }
    if (r.y + dy < 0) {
        dy = -r.y;
    }
    return {dx, dy};
}

} // namespace

TransformEachCommand::TransformEachCommand(std::size_t page, TransformList transforms)
    : m_page(page), m_transforms(std::move(transforms))
{}

void TransformEachCommand::redo(Document& document, Changes& changes)
{
    m_before.clear();
    Page& page = document.page(m_page);
    for (const auto& [id, absolute] : m_transforms) {
        const std::size_t rank = indexOf(page, id);
        if (rank >= page.objects.size() || sameTransform(page.objects[rank].common.transform, absolute)) {
            continue;
        }
        PageObject& object = page.objects[rank];
        const RectF before = boundsOnPage(object);
        m_before.emplace_back(id, object.common.transform);
        object.common.transform = absolute;
        changes.push_back(
            Change{Change::Kind::ObjectChanged, m_page, id, unitedRect(before, boundsOnPage(object))});
    }
}

void TransformEachCommand::undo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    for (auto it = m_before.rbegin(); it != m_before.rend(); ++it) {
        const std::size_t rank = indexOf(page, it->first);
        if (rank >= page.objects.size()) {
            continue;
        }
        PageObject& object = page.objects[rank];
        const RectF before = boundsOnPage(object);
        object.common.transform = it->second;
        changes.push_back(
            Change{Change::Kind::ObjectChanged, m_page, it->first, unitedRect(before, boundsOnPage(object))});
    }
}

TransformList alignTransforms(const Page& page, const std::vector<ObjectId>& ids, AlignMode mode)
{
    const std::vector<Unit> all = units(page, ids);
    if (all.size() < 2) {
        return {}; // D6 : un objet seul, ou un seul groupe, n'est pas aligné
    }
    RectF u = all.front().bounds;
    double x1 = u.x + u.width;
    double y1 = u.y + u.height;
    for (const Unit& unit : all) {
        u.x = std::min(u.x, unit.bounds.x);
        u.y = std::min(u.y, unit.bounds.y);
        x1 = std::max(x1, unit.bounds.x + unit.bounds.width);
        y1 = std::max(y1, unit.bounds.y + unit.bounds.height);
    }
    u.width = x1 - u.x;
    u.height = y1 - u.y;
    const Unit* ref = &*std::min_element(all.begin(), all.end(), [](const Unit& a, const Unit& b) {
        return a.head->common.id < b.head->common.id; // le premier créé
    });
    const RectF rb = ref->bounds;
    const auto angleOf = [](const Unit& unit) {
        return unit.group ? 0.0 : rotationDegrees(unit.head->common.transform);
    };
    TransformList out;
    for (const Unit& unit : all) {
        if (&unit == ref && (mode == AlignMode::Width || mode == AlignMode::Height ||
                             mode == AlignMode::Angle || mode == AlignMode::All)) {
            continue;
        }
        const RectF b = unit.bounds;
        switch (mode) {
        case AlignMode::Left:
            pushUnitDelta(out, unit, translation(u.x - b.x, 0));
            break;
        case AlignMode::CenterX:
            pushUnitDelta(out, unit, translation(u.center().x - b.center().x, 0));
            break;
        case AlignMode::Right:
            pushUnitDelta(out, unit, translation((u.x + u.width) - (b.x + b.width), 0));
            break;
        case AlignMode::Top:
            pushUnitDelta(out, unit, translation(0, u.y - b.y));
            break;
        case AlignMode::CenterY:
            pushUnitDelta(out, unit, translation(0, u.center().y - b.center().y));
            break;
        case AlignMode::Bottom:
            pushUnitDelta(out, unit, translation(0, (u.y + u.height) - (b.y + b.height)));
            break;
        case AlignMode::Width:
            if (b.width > kEps && rb.width > kEps) {
                pushUnitDelta(out, unit, scalingAbout(rb.width / b.width, 1.0, {b.x, b.y}));
            }
            break;
        case AlignMode::Height:
            if (b.height > kEps && rb.height > kEps) {
                pushUnitDelta(out, unit, scalingAbout(1.0, rb.height / b.height, {b.x, b.y}));
            }
            break;
        case AlignMode::Angle:
            pushUnitDelta(out, unit, rotationAbout(angleOf(*ref) - angleOf(unit), b.center()));
            break;
        case AlignMode::All:
            if (!unit.group && !ref->group) { // la taille et l'angle d'un groupe ne sont pas définis
                push(out, *unit.head, matchingAll(*unit.head, *ref->head));
            }
            break;
        }
    }
    return out;
}

TransformList fitTransforms(const Page& page, const std::vector<ObjectId>& ids, FitMode mode)
{
    TransformList out;
    for (const Unit& unit : units(page, ids)) {
        const RectF b = unit.bounds;
        if (mode == FitMode::OriginalSize) {
            if (unit.group) {
                continue; // un groupe n'a pas de taille d'origine
            }
            const PageObject* object = unit.head;
            const Transform& t = object->common.transform;
            const double sx = std::hypot(t.m11, t.m12);
            const double sy = std::hypot(t.m21, t.m22);
            if (sx < kEps || sy < kEps || (std::abs(sx - 1) < kEps && std::abs(sy - 1) < kEps)) {
                continue;
            }
            Transform n = t;
            n.m11 /= sx;
            n.m12 /= sx;
            n.m21 /= sy;
            n.m22 /= sy;
            const RectF after = boundsOnPage(withTransform(*object, n));
            n.dx += b.center().x - after.center().x;
            n.dy += b.center().y - after.center().y;
            push(out, *object, n);
            continue;
        }
        if (b.width < kEps || b.height < kEps) {
            continue;
        }
        double scale = 1.0;
        if (mode == FitMode::Width && b.width > page.width) {
            scale = page.width / b.width;
        } else if (mode == FitMode::Height && b.height > page.height) {
            scale = page.height / b.height;
        } else if (mode == FitMode::Page && (b.width > page.width || b.height > page.height)) {
            scale = std::min(page.width / b.width, page.height / b.height);
        }
        if (scale >= 1.0) {
            continue; // seuls les objets plus grands que la page sont réduits (D7)
        }
        const Transform shrink = scalingAbout(scale, scale, b.center());
        std::vector<PageObject> shrunk;
        shrunk.reserve(unit.parts.size());
        for (const PageObject* part : unit.parts) {
            shrunk.push_back(withTransform(*part, compose(part->common.transform, shrink)));
        }
        std::vector<const PageObject*> shrunkPointers;
        for (const PageObject& part : shrunk) {
            shrunkPointers.push_back(&part);
        }
        const PointF shift = shiftInto(unionRects(shrunkPointers), page.width, page.height);
        pushUnitDelta(out, unit, compose(shrink, translation(shift.x, shift.y)));
    }
    return out;
}

TransformList flipTransforms(const Page& page, const std::vector<ObjectId>& ids, Axis axis)
{
    TransformList out;
    for (const Unit& unit : units(page, ids)) {
        const PointF c = unit.bounds.center();
        pushUnitDelta(out, unit, axis == Axis::X ? reflection(false, c.y) : reflection(true, c.x));
    }
    return out;
}

TransformList originFlipTransforms(const Page& page, const std::vector<ObjectId>& ids, Axis axis,
                                   PointF origin)
{
    TransformList out;
    const Transform delta = axis == Axis::X ? reflection(false, origin.y) : reflection(true, origin.x);
    for (const Unit& unit : units(page, ids)) {
        pushUnitDelta(out, unit, delta);
    }
    return out;
}

std::vector<PageObject> mirrorCopies(const Page& page, const std::vector<ObjectId>& ids, Axis axis)
{
    // Objets choisis et membres de leurs groupes, dans l'ordre de la page (la fermeture de L5-T4 n'est pas
    // requise : preparePaste remappe les références).
    std::vector<std::size_t> ranks;
    for (const ObjectId id : expandGroups(page, ids)) {
        const std::size_t rank = indexOf(page, id);
        if (rank < page.objects.size()) {
            ranks.push_back(rank);
        }
    }
    std::sort(ranks.begin(), ranks.end());
    ranks.erase(std::unique(ranks.begin(), ranks.end()), ranks.end());
    std::vector<PageObject> copies;
    for (const std::size_t rank : ranks) {
        copies.push_back(page.objects[rank]);
    }
    bool first = true;
    double edge = 0;
    for (const PageObject& object : copies) {
        if (!hasExtent(object)) {
            continue;
        }
        const RectF b = boundsOnPage(object);
        const double v = axis == Axis::X ? b.y : b.x;
        edge = first ? v : std::min(edge, v);
        first = false;
    }
    if (first) {
        return {};
    }
    const Transform delta = axis == Axis::X ? reflection(false, edge) : reflection(true, edge);
    for (PageObject& object : copies) {
        if (hasExtent(object)) {
            object.common.transform = compose(object.common.transform, delta);
        }
    }
    return copies;
}

} // namespace actcraft::core
