#include <actcraft/core/edits.h>
#include <actcraft/core/gridsnap.h>
#include <actcraft/core/textedits.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace actcraft::core {

namespace {

constexpr double kPi = 3.14159265358979323846;
// Garde-fou : nombre maximal d'itérations par famille de lignes inclinées (pas minuscule, angle très aigu).
constexpr long long kMaxFamilySteps = 4096;

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

// Niveau « normal » : arrondi de (p − début) / pas sur chaque axe dessiné ; l'axe non dessiné garde p.
PointF nearestOnNormal(const GridLevel& level, PointF p, double sx, double sy) noexcept
{
    PointF q = p;
    if (level.linesAlongX) {
        q.x = level.start.x + std::nearbyint((p.x - level.start.x) / sx) * sx;
    }
    if (level.linesAlongY) {
        q.y = level.start.y + std::nearbyint((p.y - level.start.y) / sy) * sy;
    }
    return q;
}

// Niveaux « 45° » et « angle » : croisement le plus proche des deux familles inclinées, parmi ceux de la zone
// de rayon deux pas autour de p (même géométrie que le peintre : a + t = b − t, t = (y − début Y)·cot).
std::optional<PointF> nearestOnDiagonal(const GridLevel& level, PointF p, double sx, double sy) noexcept
{
    const double degrees =
        level.pattern == GridPattern::Diagonal45 ? 45.0 : std::clamp(level.angle, 1.0, 89.0);
    const double cot = 1.0 / std::tan(degrees * kPi / 180.0);
    const double r = 2 * std::max(sx, sy);
    const double xLo = p.x - r;
    const double xHi = p.x + r;
    const double yLo = p.y - r;
    const double yHi = p.y + r;
    const double tLo = (yLo - level.start.y) * cot;
    const double tHi = (yHi - level.start.y) * cot;

    std::optional<PointF> best;
    double bestDistance = std::numeric_limits<double>::infinity();
    // a + t doit tomber dans [xLo, xHi] pour t dans [tLo, tHi].
    const auto iFirst = static_cast<long long>(std::ceil((xLo - tHi - level.start.x) / sx - 1e-9));
    const auto iLast = std::min(static_cast<long long>(std::floor((xHi - tLo - level.start.x) / sx + 1e-9)),
                                iFirst + kMaxFamilySteps);
    for (long long i = iFirst; i <= iLast; ++i) {
        const double a = level.start.x + static_cast<double>(i) * sx;
        // b − a = 2t, t parcourt [tLo, tHi].
        const auto jFirst = static_cast<long long>(std::ceil((a + 2 * tLo - level.start.x) / sy - 1e-9));
        const auto jLast =
            std::min(static_cast<long long>(std::floor((a + 2 * tHi - level.start.x) / sy + 1e-9)),
                     jFirst + kMaxFamilySteps);
        for (long long j = jFirst; j <= jLast; ++j) {
            const double b = level.start.x + static_cast<double>(j) * sy;
            const double t = (b - a) / 2;
            const PointF q{a + t, level.start.y + t / cot};
            if (q.x < xLo || q.x > xHi || q.y < yLo || q.y > yHi) {
                continue;
            }
            const double d = squaredDistance(q, p);
            if (d < bestDistance) {
                best = q;
                bestDistance = d;
            }
        }
    }
    return best;
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
