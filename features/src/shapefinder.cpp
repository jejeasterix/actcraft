#include <actcraft/features/shapefinder.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <numbers>

// Reconnaissance de forme (F-296), algorithme à nous. Le tracé est classé par sa géométrie propre (coins,
// côtés, axes principaux), jamais par sa boîte englobante : une forme inclinée garde sa nature et son angle.
//
// Seuils à nous, réglés sur nos tracés synthétiques (tst_features_shapefinder et un balayage de graines) ;
// chacun sert une seule décision, avec sa raison.

namespace actcraft::features {

namespace {

// Points du tracé refermé après ré-échantillonnage à pas constant : une dizaine par côté de pentagone, assez
// peu pour que le tremblement de la main ne fasse pas de faux coins.
constexpr std::size_t kResampleCount = 64;
// Diagonale de la boîte en dessous de laquelle le trait est une touche ou un point, pas une forme (taille
// seulement, jamais pour classer).
constexpr double kMinExtent = 16.0;
// Ligne : écart maximal à la corde, en part de la corde (la main tremble de quelques pixels sur une ligne).
constexpr double kLineMaxDeviation = 0.04;
// Ligne : la corde fait au moins cette part de la longueur (sinon le trait revient sur lui-même).
constexpr double kLineMinChordRatio = 0.85;
// Forme fermée : l'écart entre la fin et le départ ne dépasse pas cette part de la longueur.
constexpr double kClosureMaxGap = 0.20;
// Le tracé se referme au point de son dernier quart le plus proche du départ ; ce qui suit est un dépassement
// de la main, retiré (sinon le segment de fermeture repart en arrière et fausse le compte des tours).
constexpr double kOvershootSearch = 0.25;
// Coin : virage cumulé sur une fenêtre de 5 points (virages des trois points intérieurs) d'au moins 55°. Un
// coin de pentagone (72°) passe ; un arc de cercle (≈ 17° par fenêtre) et le bruit de la main (≈ 10°) non.
constexpr double kCornerMinTurnDegrees = 55.0;
// Deux séries de points « coin » séparées par au plus deux points sont un seul coin arrondi.
constexpr std::size_t kCornerMergeGap = 2;
// Un coin se recale sur le point du tracé le plus proche du sommet ajusté, à deux pas au plus (demi-fenêtre).
constexpr std::size_t kCornerSnapSteps = 2;
// Un sommet ajusté (intersection des droites de ses deux côtés) à plus d'un pas du tracé : les « côtés » sont
// des arcs qui se coupent loin du tracé (ellipse allongée), pas les côtés d'un polygone.
constexpr double kVertexMaxOffsetSteps = 1.0;
// Côté droit : écart maximal à son segment, en part de sa longueur.
constexpr double kSideMaxDeviation = 0.08;
// Rectangle : chaque angle à 90° ± 10° ; un trapèze dessiné à main levée (76°, 104°) reste un polygone (P16).
constexpr double kRightAngleSlackDegrees = 10.0;
// Carré : rapport des côtés du tracé (petit / grand) d'au moins 0,85.
constexpr double kSquareMinRatio = 0.85;
// Polygone : au plus 6 coins. L'angle extérieur d'un polygone de 7 côtés ou plus (≤ 51°) est sous
// kCornerMinTurnDegrees : ses sommets ne sont pas des coins. La borne suit le seuil des coins et change avec
// lui.
constexpr std::size_t kMaxPolygonCorners = 6;
// Ellipse : écart radial normalisé moyen à l'ellipse ajustée (|r / r attendu − 1|) d'au plus 0,12 ; un carré
// (≈ 0,15) et une spirale le dépassent.
constexpr double kEllipseMaxRadialError = 0.12;
// Cercle : rapport des demi-axes principaux (petit / grand) d'au moins 0,85.
constexpr double kCircleMinRatio = 0.85;
// Garde numérique : deux directions unitaires dont le produit vectoriel est sous ce seuil sont parallèles.
constexpr double kParallelEpsilon = 1e-9;

using Points = std::vector<QPointF>;

double toDegrees(double radians)
{
    return radians * 180.0 / std::numbers::pi;
}

double toRadians(double degrees)
{
    return degrees * std::numbers::pi / 180.0;
}

double cross(QPointF a, QPointF b)
{
    return a.x() * b.y() - a.y() * b.x();
}

double distance(QPointF a, QPointF b)
{
    return std::hypot(a.x() - b.x(), a.y() - b.y());
}

double segmentDistance(QPointF p, QPointF a, QPointF b)
{
    const QPointF d = b - a;
    const double squared = QPointF::dotProduct(d, d);
    if (squared <= 0) {
        return distance(p, a);
    }
    const double t = std::clamp(QPointF::dotProduct(p - a, d) / squared, 0.0, 1.0);
    return distance(p, a + d * t);
}

double pathLength(const Points& pts)
{
    double total = 0;
    for (std::size_t i = 1; i < pts.size(); ++i) {
        total += distance(pts[i - 1], pts[i]);
    }
    return total;
}

// Rotation dans le sens de core::rotation (horaire à l'écran, y vers le bas).
QPointF rotated(QPointF v, double radians)
{
    const double c = std::cos(radians);
    const double s = std::sin(radians);
    return {c * v.x() - s * v.y(), s * v.x() + c * v.y()};
}

// Virage signé de a→b vers b→c, en degrés, dans ]−180, 180].
double turnDegrees(QPointF a, QPointF b, QPointF c)
{
    const QPointF d1 = b - a;
    const QPointF d2 = c - b;
    return toDegrees(std::atan2(cross(d1, d2), QPointF::dotProduct(d1, d2)));
}

QRectF boundsOf(const Points& pts)
{
    double minX = pts.front().x();
    double maxX = minX;
    double minY = pts.front().y();
    double maxY = minY;
    for (const QPointF& p : pts) {
        minX = std::min(minX, p.x());
        maxX = std::max(maxX, p.x());
        minY = std::min(minY, p.y());
        maxY = std::max(maxY, p.y());
    }
    return QRectF(QPointF(minX, minY), QPointF(maxX, maxY));
}

double extent(const Points& pts)
{
    const QRectF box = boundsOf(pts);
    return std::hypot(box.width(), box.height());
}

bool isLine(const Points& pts, double length)
{
    const QPointF a = pts.front();
    const QPointF b = pts.back();
    const double chord = distance(a, b);
    if (chord < kLineMinChordRatio * length) {
        return false;
    }
    return std::all_of(pts.begin(), pts.end(),
                       [&](QPointF p) { return segmentDistance(p, a, b) <= kLineMaxDeviation * chord; });
}

// Le tracé refermé sur son départ, dépassement retiré ; rien s'il reste ouvert.
std::optional<Points> closure(const Points& pts, double length)
{
    const QPointF start = pts.front();
    double bestGap = distance(pts.back(), start);
    std::size_t bestSegment = pts.size() - 2;
    QPointF bestPoint = pts.back();
    double travelled = 0;
    for (std::size_t i = 1; i < pts.size(); ++i) {
        const QPointF a = pts[i - 1];
        const QPointF b = pts[i];
        const double span = distance(a, b);
        travelled += span;
        if (travelled < (1 - kOvershootSearch) * length || span <= 0) {
            continue;
        }
        const double t = std::clamp(QPointF::dotProduct(start - a, b - a) / (span * span), 0.0, 1.0);
        const QPointF q = a + (b - a) * t;
        const double gap = distance(q, start);
        if (gap < bestGap) {
            bestGap = gap;
            bestSegment = i - 1;
            bestPoint = q;
        }
    }
    if (bestGap > kClosureMaxGap * length) {
        return std::nullopt;
    }
    Points loop(pts.begin(), pts.begin() + static_cast<std::ptrdiff_t>(bestSegment + 1));
    loop.push_back(bestPoint);
    return loop;
}

// kResampleCount points à pas constant le long du tracé refermé (segment de fermeture compris), lus en
// anneau.
Points resample(const Points& loop)
{
    Points closed = loop;
    closed.push_back(loop.front());
    const double step = pathLength(closed) / static_cast<double>(kResampleCount);
    Points ring;
    ring.reserve(kResampleCount);
    ring.push_back(closed.front());
    std::size_t segment = 0;
    double segmentStart = 0;
    for (std::size_t k = 1; k < kResampleCount; ++k) {
        const double target = static_cast<double>(k) * step;
        while (segment + 2 < closed.size() &&
               segmentStart + distance(closed[segment], closed[segment + 1]) < target) {
            segmentStart += distance(closed[segment], closed[segment + 1]);
            ++segment;
        }
        const QPointF a = closed[segment];
        const QPointF b = closed[segment + 1];
        const double span = distance(a, b);
        ring.push_back(span > 0 ? a + (b - a) * ((target - segmentStart) / span) : a);
    }
    return ring;
}

std::vector<double> turnsAt(const Points& ring)
{
    const std::size_t n = ring.size();
    std::vector<double> turns(n);
    for (std::size_t i = 0; i < n; ++i) {
        turns[i] = turnDegrees(ring[(i + n - 1) % n], ring[i], ring[(i + 1) % n]);
    }
    return turns;
}

// Nombre de tours du tracé refermé (signé : sens du tracé).
int windingTurns(const std::vector<double>& turns)
{
    double total = 0;
    for (const double t : turns) {
        total += t;
    }
    return static_cast<int>(std::lround(total / 360.0));
}

// Virage cumulé sur la fenêtre de 5 points centrée sur chaque point.
std::vector<double> windowTurns(const std::vector<double>& turns)
{
    const std::size_t n = turns.size();
    std::vector<double> window(n);
    for (std::size_t i = 0; i < n; ++i) {
        window[i] = turns[(i + n - 1) % n] + turns[i] + turns[(i + 1) % n];
    }
    return window;
}

// Indices des coins, croissants ; chaque série de points « coin » (voisines fusionnées) donne son point du
// milieu.
std::vector<std::size_t> corners(const std::vector<double>& window)
{
    const std::size_t n = window.size();
    const auto isCorner = [&](std::size_t i) { return std::abs(window[i]) >= kCornerMinTurnDegrees; };
    std::size_t start = n;
    for (std::size_t i = 0; i < n; ++i) {
        if (!isCorner(i)) {
            start = i;
            break;
        }
    }
    if (start == n) {
        return {}; // tout le tracé tourne : une boucle serrée, sans coin
    }
    std::vector<std::vector<std::size_t>> runs;
    std::size_t gap = n;
    for (std::size_t k = 1; k <= n; ++k) {
        const std::size_t i = (start + k) % n;
        if (!isCorner(i)) {
            ++gap;
            continue;
        }
        if (!runs.empty() && gap <= kCornerMergeGap) {
            runs.back().push_back(i);
        } else {
            runs.push_back({i});
        }
        gap = 0;
    }
    if (runs.size() >= 2 && (runs.front().front() + n - runs.back().back()) % n - 1 <= kCornerMergeGap) {
        runs.back().insert(runs.back().end(), runs.front().begin(), runs.front().end());
        runs.front() = runs.back();
        runs.pop_back();
    }
    std::vector<std::size_t> result;
    for (const auto& run : runs) {
        std::size_t middle = run.size() / 2;
        if (run.size() % 2 == 0 && std::abs(window[run[middle - 1]]) >= std::abs(window[run[middle]])) {
            --middle;
        }
        result.push_back(run[middle]);
    }
    std::sort(result.begin(), result.end());
    return result;
}

// Points strictement entre deux coins, dans le sens du tracé.
Points sideInterior(const Points& ring, std::size_t from, std::size_t to)
{
    const std::size_t n = ring.size();
    Points out;
    for (std::size_t i = (from + 1) % n; i != to; i = (i + 1) % n) {
        out.push_back(ring[i]);
    }
    return out;
}

struct Axes
{
    QPointF mean;
    QPointF center;     // milieu des écarts extrêmes projetés
    double radians = 0; // direction de l'axe principal
    double halfU = 0;   // demi-étendue le long de l'axe principal
    double halfV = 0;   // demi-étendue le long de l'axe secondaire
};

// Axes principaux (analyse en composantes principales des points) et demi-étendues projetées.
Axes principalAxes(const Points& pts)
{
    Axes axes;
    for (const QPointF& p : pts) {
        axes.mean += p;
    }
    axes.mean /= static_cast<double>(pts.size());
    double sxx = 0;
    double syy = 0;
    double sxy = 0;
    for (const QPointF& p : pts) {
        const QPointF d = p - axes.mean;
        sxx += d.x() * d.x();
        syy += d.y() * d.y();
        sxy += d.x() * d.y();
    }
    axes.radians = 0.5 * std::atan2(2 * sxy, sxx - syy);
    const QPointF u(std::cos(axes.radians), std::sin(axes.radians));
    const QPointF v(-u.y(), u.x());
    double minU = std::numeric_limits<double>::max();
    double maxU = std::numeric_limits<double>::lowest();
    double minV = minU;
    double maxV = maxU;
    for (const QPointF& p : pts) {
        const QPointF d = p - axes.mean;
        const double pu = QPointF::dotProduct(d, u);
        const double pv = QPointF::dotProduct(d, v);
        minU = std::min(minU, pu);
        maxU = std::max(maxU, pu);
        minV = std::min(minV, pv);
        maxV = std::max(maxV, pv);
    }
    axes.center = axes.mean + u * ((minU + maxU) / 2) + v * ((minV + maxV) / 2);
    axes.halfU = (maxU - minU) / 2;
    axes.halfV = (maxV - minV) / 2;
    return axes;
}

// Écart radial normalisé moyen à l'ellipse des axes principaux.
double radialError(const Points& pts, const Axes& axes)
{
    if (axes.halfU <= 0 || axes.halfV <= 0) {
        return std::numeric_limits<double>::infinity();
    }
    const QPointF u(std::cos(axes.radians), std::sin(axes.radians));
    const QPointF v(-u.y(), u.x());
    double total = 0;
    for (const QPointF& p : pts) {
        const QPointF d = p - axes.center;
        total += std::abs(
            std::hypot(QPointF::dotProduct(d, u) / axes.halfU, QPointF::dotProduct(d, v) / axes.halfV) - 1);
    }
    return total / static_cast<double>(pts.size());
}

struct SideLine
{
    QPointF point;
    QPointF direction; // unitaire, dans le sens du tracé
};

// Droite ajustée sur chaque côté (axe principal de ses points).
std::vector<SideLine> sideLines(const Points& ring, const std::vector<std::size_t>& cs)
{
    std::vector<SideLine> lines;
    for (std::size_t k = 0; k < cs.size(); ++k) {
        const std::size_t from = cs[k];
        const std::size_t to = cs[(k + 1) % cs.size()];
        Points pts = sideInterior(ring, from, to);
        if (pts.size() < 2) {
            pts.insert(pts.begin(), ring[from]);
            pts.push_back(ring[to]);
        }
        const Axes axes = principalAxes(pts);
        QPointF direction(std::cos(axes.radians), std::sin(axes.radians));
        if (QPointF::dotProduct(direction, pts.back() - pts.front()) < 0) {
            direction = -direction;
        }
        lines.push_back({axes.mean, direction});
    }
    return lines;
}

// Sommet ajusté de chaque coin : intersection des droites de ses deux côtés.
Points vertices(const Points& ring, const std::vector<std::size_t>& cs, const std::vector<SideLine>& lines)
{
    const std::size_t m = cs.size();
    Points verts;
    for (std::size_t k = 0; k < m; ++k) {
        const SideLine& before = lines[(k + m - 1) % m];
        const SideLine& after = lines[k];
        const double den = cross(before.direction, after.direction);
        if (std::abs(den) < kParallelEpsilon) {
            verts.push_back(ring[cs[k]]);
            continue;
        }
        const double t = cross(after.point - before.point, after.direction) / den;
        verts.push_back(before.point + before.direction * t);
    }
    return verts;
}

// Chaque coin recalé sur le point du tracé le plus proche de son sommet ajusté.
std::vector<std::size_t> snapCorners(const Points& ring, const std::vector<std::size_t>& cs,
                                     const Points& verts)
{
    const std::size_t n = ring.size();
    std::vector<std::size_t> snapped;
    for (std::size_t k = 0; k < cs.size(); ++k) {
        std::size_t best = cs[k];
        for (std::size_t d = 0; d <= 2 * kCornerSnapSteps; ++d) {
            const std::size_t i = (cs[k] + n - kCornerSnapSteps + d) % n;
            if (distance(ring[i], verts[k]) < distance(ring[best], verts[k])) {
                best = i;
            }
        }
        snapped.push_back(best);
    }
    return snapped;
}

// Côtés droits : chaque sommet ajusté est près du tracé, chaque côté près du segment de ses deux sommets.
bool sidesStraight(const Points& ring, const std::vector<std::size_t>& cs, const Points& verts)
{
    const std::size_t m = cs.size();
    const double step =
        (pathLength(ring) + distance(ring.back(), ring.front())) / static_cast<double>(ring.size());
    for (std::size_t k = 0; k < m; ++k) {
        if (distance(verts[k], ring[cs[k]]) > kVertexMaxOffsetSteps * step) {
            return false;
        }
    }
    for (std::size_t k = 0; k < m; ++k) {
        const QPointF a = verts[k];
        const QPointF b = verts[(k + 1) % m];
        const double side = distance(a, b);
        if (side <= 0) {
            return false;
        }
        for (const QPointF& p : sideInterior(ring, cs[k], cs[(k + 1) % m])) {
            if (segmentDistance(p, a, b) > kSideMaxDeviation * side) {
                return false;
            }
        }
    }
    return true;
}

std::optional<RecognizedShape> rectangleShape(const std::vector<SideLine>& lines, const Points& verts)
{
    for (std::size_t k = 0; k < 4; ++k) {
        const double cosine =
            std::clamp(QPointF::dotProduct(lines[(k + 3) % 4].direction, lines[k].direction), -1.0, 1.0);
        if (std::abs(toDegrees(std::acos(cosine)) - 90.0) > kRightAngleSlackDegrees) {
            return std::nullopt;
        }
    }
    // Angle du premier côté ramené dans ]−45, 45] ; un nombre impair de quarts de tour échange largeur et
    // hauteur.
    double angle = toDegrees(std::atan2(lines[0].direction.y(), lines[0].direction.x()));
    int quarters = 0;
    while (angle > 45) {
        angle -= 90;
        ++quarters;
    }
    while (angle <= -45) {
        angle += 90;
        ++quarters;
    }
    double width = (distance(verts[0], verts[1]) + distance(verts[2], verts[3])) / 2;
    double height = (distance(verts[1], verts[2]) + distance(verts[3], verts[0])) / 2;
    if (quarters % 2 != 0) {
        std::swap(width, height);
    }
    RecognizedShape shape;
    shape.kind = RecognizedShape::Kind::Rectangle;
    if (std::min(width, height) / std::max(width, height) >= kSquareMinRatio) {
        shape.kind = RecognizedShape::Kind::Square;
        width = (width + height) / 2;
        height = width;
    }
    QPointF center;
    for (const QPointF& v : verts) {
        center += v;
    }
    center /= 4.0;
    shape.bounds = QRectF(center.x() - width / 2, center.y() - height / 2, width, height);
    shape.angleDegrees = angle;
    const double radians = toRadians(angle);
    for (const QPointF corner : {QPointF(-width / 2, -height / 2), QPointF(width / 2, -height / 2),
                                 QPointF(width / 2, height / 2), QPointF(-width / 2, height / 2)}) {
        shape.corners.push_back(center + rotated(corner, radians));
    }
    return shape;
}

std::optional<RecognizedShape> polygonShape(const Points& ring, const std::vector<std::size_t>& cs,
                                            const std::vector<double>& window, int winding)
{
    if (cs.size() < 3 || cs.size() > kMaxPolygonCorners) {
        return std::nullopt;
    }
    // Convexe : chaque coin tourne dans le sens du tracé ; un coin à rebours est celui d'un gribouillis qui
    // se croise.
    for (const std::size_t c : cs) {
        if (window[c] * static_cast<double>(winding) <= 0) {
            return std::nullopt;
        }
    }
    const std::vector<SideLine> lines = sideLines(ring, cs);
    const Points verts = vertices(ring, cs, lines);
    const std::vector<std::size_t> snapped = snapCorners(ring, cs, verts);
    std::vector<std::size_t> distinct = snapped;
    std::sort(distinct.begin(), distinct.end());
    if (std::adjacent_find(distinct.begin(), distinct.end()) != distinct.end() ||
        !sidesStraight(ring, snapped, verts)) {
        return std::nullopt;
    }
    if (cs.size() == 4) {
        if (auto rectangle = rectangleShape(lines, verts)) {
            return rectangle;
        }
    }
    RecognizedShape shape;
    shape.kind = cs.size() == 3 ? RecognizedShape::Kind::Triangle : RecognizedShape::Kind::Polygon;
    shape.bounds = boundsOf(verts);
    shape.corners = verts;
    return shape;
}

std::optional<RecognizedShape> ellipseShape(const Points& ring)
{
    const Axes axes = principalAxes(ring);
    if (radialError(ring, axes) > kEllipseMaxRadialError) {
        return std::nullopt;
    }
    double major = axes.halfU;
    double minor = axes.halfV;
    double angle = toDegrees(axes.radians);
    if (minor > major) {
        std::swap(major, minor);
        angle += 90;
    }
    // Axe principal ramené dans ]−90, 90].
    while (angle > 90) {
        angle -= 180;
    }
    while (angle <= -90) {
        angle += 180;
    }
    RecognizedShape shape;
    if (minor / major >= kCircleMinRatio) {
        const double diameter = major + minor;
        shape.kind = RecognizedShape::Kind::Circle;
        shape.bounds =
            QRectF(axes.center.x() - diameter / 2, axes.center.y() - diameter / 2, diameter, diameter);
        return shape;
    }
    shape.kind = RecognizedShape::Kind::Ellipse;
    shape.bounds = QRectF(axes.center.x() - major, axes.center.y() - minor, 2 * major, 2 * minor);
    shape.angleDegrees = angle;
    return shape;
}

RecognizedShape lineShape(QPointF a, QPointF b)
{
    RecognizedShape shape;
    shape.kind = RecognizedShape::Kind::Line;
    const double chord = distance(a, b);
    const QPointF middle = (a + b) / 2.0;
    shape.bounds = QRectF(middle.x() - chord / 2, middle.y(), chord, 0);
    shape.angleDegrees = toDegrees(std::atan2(b.y() - a.y(), b.x() - a.x()));
    shape.corners = {a, b};
    return shape;
}

} // namespace

std::optional<RecognizedShape> recognizeShape(const std::vector<QPointF>& points)
{
    if (points.size() < 2 || extent(points) < kMinExtent) {
        return std::nullopt;
    }
    const double length = pathLength(points);
    if (length <= 0) {
        return std::nullopt;
    }
    if (isLine(points, length)) {
        return lineShape(points.front(), points.back());
    }
    const auto loop = closure(points, length);
    if (!loop) {
        return std::nullopt;
    }
    const Points ring = resample(*loop);
    const std::vector<double> turns = turnsAt(ring);
    const int winding = windingTurns(turns);
    // Une forme fermée fait exactement un tour : une spirale en fait deux, un gribouillis qui se croise
    // souvent zéro.
    if (std::abs(winding) != 1) {
        return std::nullopt;
    }
    const std::vector<double> window = windowTurns(turns);
    if (auto polygon = polygonShape(ring, corners(window), window, winding)) {
        return polygon;
    }
    return ellipseShape(ring);
}

} // namespace actcraft::features
