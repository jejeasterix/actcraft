#include <actcraft/core/edits.h>
#include <actcraft/core/gridsnap.h>
#include <actcraft/core/textedits.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace actcraft::core {

namespace {

constexpr double kPi = 3.14159265358979323846;
// Garde-fou de la réduction de base : quelques tours suffisent (le nombre dépend des seuls pas et angle du
// niveau, au plus logarithmique en leur rapport) ; la borne n'est là que contre un arrondi flottant
// pathologique.
constexpr int kMaxReductionSteps = 64;

struct Vec
{
    double x = 0;
    double y = 0;
};

double dot(Vec a, Vec b) noexcept
{
    return a.x * b.x + a.y * b.y;
}

double squaredDistance(PointF a, PointF b) noexcept
{
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return dx * dx + dy * dy;
}

// Pas effectif d'un axe : pas × « Échelle par défaut », au moins 1 (comme le peintre).
double effectiveStep(double spacing, double baseZoom) noexcept
{
    return std::max(spacing * baseZoom, 1.0);
}

// Ligne la plus proche d'un axe du niveau normal : arrondi de (v − début) / pas, sans descendre sous la
// première ligne que le peintre trace (il part de max(0, début), comme rangeOf).
double snapAxis(double value, double start, double step) noexcept
{
    const double first = std::ceil((std::max(0.0, start) - start) / step - 1e-9);
    return start + std::max(std::nearbyint((value - start) / step), first) * step;
}

// Niveau « normal » : chaque axe dessiné accroche sa ligne la plus proche ; l'axe non dessiné garde p ; aucun
// axe dessiné, aucun accrochage.
std::optional<PointF> nearestOnNormal(const GridLevel& level, PointF p, double sx, double sy) noexcept
{
    if (!level.linesAlongX && !level.linesAlongY) {
        return std::nullopt;
    }
    PointF q = p;
    if (level.linesAlongX) {
        q.x = snapAxis(p.x, level.start.x, sx);
    }
    if (level.linesAlongY) {
        q.y = snapAxis(p.y, level.start.y, sy);
    }
    return q;
}

// Une seule famille inclinée (x = début X + k·pas + sens·(y − début Y)·cot) : pied de la perpendiculaire sur
// la ligne la plus proche. Les lignes sont parallèles et régulières : la plus proche est l'arrondi de l'écart
// horizontal ramené au pas.
PointF nearestOnFamily(PointF start, double sign, double step, double cot, PointF p) noexcept
{
    const double offset = p.x - start.x - sign * (p.y - start.y) * cot;
    const Vec origin{start.x + std::nearbyint(offset / step) * step, start.y};
    const Vec direction{sign * cot, 1};
    const double along = dot({p.x - origin.x, p.y - origin.y}, direction) / dot(direction, direction);
    return {origin.x + along * direction.x, origin.y + along * direction.y};
}

// Point du réseau origin + m·b1 + n·b2 (m, n entiers) le plus proche de p, en temps constant : réduction de
// Lagrange-Gauss (même réseau, |b1| ≤ |b2|, |b1·b2| ≤ |b1|²/2), puis coordonnées de p dans la base réduite.
// Avec une base réduite, la maille qui contient p se coupe en deux triangles non obtus, et le point du réseau
// le plus proche est un sommet de cette maille ; les 3 × 3 voisins de l'arrondi la couvrent. Arrondir les
// coordonnées dans la base d'origine ne suffit pas : à 89°, elle donne un croisement d'une autre rangée, à
// des centaines de points.
PointF nearestLatticePoint(PointF origin, Vec b1, Vec b2, PointF p) noexcept
{
    for (int pass = 0; pass < kMaxReductionSteps; ++pass) {
        if (dot(b1, b1) > dot(b2, b2)) {
            std::swap(b1, b2);
        }
        const double mu = dot(b1, b2) / dot(b1, b1);
        if (std::abs(mu) <= 0.5) {
            break;
        }
        const double m = std::nearbyint(mu);
        b2 = {b2.x - m * b1.x, b2.y - m * b1.y};
    }
    const Vec d{p.x - origin.x, p.y - origin.y};
    const double det = b1.x * b2.y - b1.y * b2.x;
    const double u = std::nearbyint((d.x * b2.y - d.y * b2.x) / det);
    const double v = std::nearbyint((b1.x * d.y - b1.y * d.x) / det);
    PointF best = origin;
    double bestDistance = std::numeric_limits<double>::infinity();
    for (int du = -1; du <= 1; ++du) {
        for (int dv = -1; dv <= 1; ++dv) {
            const double m = u + static_cast<double>(du);
            const double n = v + static_cast<double>(dv);
            const PointF q{origin.x + m * b1.x + n * b2.x, origin.y + m * b1.y + n * b2.y};
            const double dist = squaredDistance(q, p);
            if (dist < bestDistance) {
                best = q;
                bestDistance = dist;
            }
        }
    }
    return best;
}

// Niveaux « 45° » et « angle », même géométrie que le peintre : famille + (tracée si linesAlongX) x = a + t,
// famille − (tracée si linesAlongY) x = b − t, t = (y − début Y)·cot, a = début X + i·pas X, b = début X +
// j·pas Y. Les croisements (a + t = b − t) forment le réseau début + i·e1 + j·e2, e1 = (pas X, −pas X / cot)
// / 2, e2 = (pas Y, pas Y / cot) / 2 : le point y est ramené et arrondi, sans parcourir de lignes.
std::optional<PointF> nearestOnDiagonal(const GridLevel& level, PointF p, double sx, double sy) noexcept
{
    const double degrees =
        level.pattern == GridPattern::Diagonal45 ? 45.0 : std::clamp(level.angle, 1.0, 89.0);
    const double cot = 1.0 / std::tan(degrees * kPi / 180.0);
    if (level.linesAlongX && level.linesAlongY) {
        return nearestLatticePoint(level.start, {sx / 2, -sx / (2 * cot)}, {sy / 2, sy / (2 * cot)}, p);
    }
    if (level.linesAlongX) {
        return nearestOnFamily(level.start, 1, sx, cot, p);
    }
    if (level.linesAlongY) {
        return nearestOnFamily(level.start, -1, sy, cot, p);
    }
    return std::nullopt;
}

// Ligne de base du texte, coordonnées de l'objet : core::textBaseline si elle répond, sinon l'origine de la
// première ligne lue directement.
std::optional<PointF> localBaseline(const Text& text) noexcept
{
    if (const auto b = textBaseline(text)) {
        return b;
    }
    if (!text.runs.empty()) {
        return text.runs.front().baseline;
    }
    return std::nullopt;
}

// Coin bas gauche de l'emprise géométrique transformée, sans la demi-épaisseur du contour (l'ancrage vise la
// forme, pas son trait) ; aucun pour les objets dont l'emprise vient de points ou d'un trait (traits, lignes,
// connecteurs, chemins) : snapAnchor retombe alors sur boundsOnPage.
std::optional<PointF> geometricBottomLeft(const PageObject& object) noexcept
{
    const Transform& t = object.common.transform;
    RectF r;
    if (const auto* s = std::get_if<Shape>(&body(object));
        s && s->kind != ShapeKind::Path && s->kind != ShapeKind::Polygon) {
        r = s->bounds;
    } else if (const auto* t2 = std::get_if<Text>(&body(object))) {
        r = t2->bounds;
    } else if (const auto* im = std::get_if<ImageObject>(&body(object))) {
        r = im->bounds;
    } else if (const auto* m = std::get_if<Media>(&body(object))) {
        r = m->bounds;
    } else {
        return std::nullopt;
    }
    const PointF corners[4] = {t.map({r.x, r.y}), t.map({r.x + r.width, r.y}), t.map({r.x, r.y + r.height}),
                               t.map({r.x + r.width, r.y + r.height})};
    double minX = corners[0].x;
    double maxY = corners[0].y;
    for (const PointF& c : corners) {
        minX = std::min(minX, c.x);
        maxY = std::max(maxY, c.y);
    }
    return PointF{minX, maxY};
}

} // namespace

std::optional<PointF> nearestGridPoint(const Grid& grid, PointF point, double, double) noexcept
{
    if (!grid.snapEnabled) {
        return std::nullopt;
    }
    std::optional<PointF> best;
    double bestDistance = std::numeric_limits<double>::infinity();
    for (const GridLevel& level : grid.levels) {
        if (!level.snap) {
            continue;
        }
        const double sx = effectiveStep(level.spacingX, grid.baseZoom);
        const double sy = effectiveStep(level.spacingY, grid.baseZoom);
        const std::optional<PointF> q = level.pattern == GridPattern::Normal
                                            ? std::optional<PointF>(nearestOnNormal(level, point, sx, sy))
                                            : nearestOnDiagonal(level, point, sx, sy);
        if (!q) {
            continue;
        }
        const double d = squaredDistance(*q, point);
        if (d < bestDistance) {
            best = q;
            bestDistance = d;
        }
    }
    return best;
}

PointF snapAnchor(const PageObject& object)
{
    if (const auto* text = std::get_if<Text>(&body(object))) {
        if (const auto baseline = localBaseline(*text)) {
            return object.common.transform.map(*baseline);
        }
    }
    if (const auto corner = geometricBottomLeft(object)) {
        return *corner;
    }
    const RectF r = boundsOnPage(object);
    return {r.x, r.y + r.height};
}

PointF snapDelta(const Page& page, const std::vector<ObjectId>& ids, PointF delta) noexcept
{
    if (!page.grid.snapEnabled) {
        return delta;
    }
    for (const PageObject& object : page.objects) {
        if (std::find(ids.begin(), ids.end(), object.common.id) == ids.end()) {
            continue;
        }
        const PointF anchor = snapAnchor(object);
        const PointF target{anchor.x + delta.x, anchor.y + delta.y};
        const auto snapped = nearestGridPoint(page.grid, target, page.width, page.height);
        if (!snapped) {
            return delta;
        }
        return {delta.x + (snapped->x - target.x), delta.y + (snapped->y - target.y)};
    }
    return delta;
}

} // namespace actcraft::core
