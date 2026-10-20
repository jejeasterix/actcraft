#include <actcraft/render/strokegeometry.h>

#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <optional>

namespace actcraft::render {

namespace {

// L'angle ramené dans ]-π, π].
double wrappedTurn(double turn) noexcept
{
    while (turn > std::numbers::pi) {
        turn -= 2 * std::numbers::pi;
    }
    while (turn <= -std::numbers::pi) {
        turn += 2 * std::numbers::pi;
    }
    return turn;
}

float f(double v)
{
    return static_cast<float>(v);
}

void appendQuad(core::PointF a, core::PointF b, double width, std::vector<Vertex2D>& out)
{
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double length = std::sqrt(dx * dx + dy * dy);
    if (length < 1e-9) {
        for (std::size_t i = 0; i < kVerticesPerSegment; ++i) {
            out.push_back({f(a.x), f(a.y)});
        }
        return;
    }
    const double half = width / 2;
    const double ux = dx / length;
    const double uy = dy / length;
    const double nx = -uy * half;
    const double ny = ux * half;
    const double ax = a.x - ux * half;
    const double ay = a.y - uy * half;
    const double bx = b.x + ux * half;
    const double by = b.y + uy * half;
    const Vertex2D p0{f(ax + nx), f(ay + ny)};
    const Vertex2D p1{f(ax - nx), f(ay - ny)};
    const Vertex2D p2{f(bx + nx), f(by + ny)};
    const Vertex2D p3{f(bx - nx), f(by - ny)};
    out.insert(out.end(), {p0, p1, p2, p2, p1, p3});
}

core::PointF pointOf(const core::StrokePoint& p)
{
    return {p.x, p.y};
}

double distance(core::PointF a, core::PointF b)
{
    return std::hypot(b.x - a.x, b.y - a.y);
}

// Courbe quadratique s → e (contrôle c) échantillonnée sans son départ : `step` unités au plus par
// sous-segment (la longueur du polygone de contrôle borne celle de la courbe).
void appendQuadratic(core::PointF s, core::PointF c, core::PointF e, double step,
                     std::vector<core::PointF>& out)
{
    const double length = distance(s, c) + distance(c, e);
    const int count = std::max(1, static_cast<int>(std::ceil(length / step)));
    for (int i = 1; i <= count; ++i) {
        const double t = static_cast<double>(i) / count;
        const double u = 1 - t;
        out.push_back(
            {u * u * s.x + 2 * u * t * c.x + t * t * e.x, u * u * s.y + 2 * u * t * c.y + t * t * e.y});
    }
}

// Côtés du polygone qui approche un cercle de rayon `radius` à 0,1 unité près (8 à 128).
int circleSides(double radius)
{
    constexpr double kTolerance = 0.1;
    if (radius <= kTolerance) {
        return 8;
    }
    const double sides = std::numbers::pi / std::acos(1 - kTolerance / radius);
    return std::clamp(static_cast<int>(std::ceil(sides)), 8, 128);
}

void appendTriangle(core::PointF a, core::PointF b, core::PointF c, std::vector<Vertex2D>& out)
{
    out.insert(out.end(), {Vertex2D{f(a.x), f(a.y)}, Vertex2D{f(b.x), f(b.y)}, Vertex2D{f(c.x), f(c.y)}});
}

core::PointF onCircle(core::PointF center, double radius, double angle)
{
    return {center.x + radius * std::cos(angle), center.y + radius * std::sin(angle)};
}

// Éventail de `center` entre les angles `from` et `from + sweep`, `sides` côtés pour un tour complet.
void appendSector(core::PointF center, double radius, double from, double sweep, int sides,
                  std::vector<Vertex2D>& out)
{
    const double perSide = 2 * std::numbers::pi / sides;
    const int count = std::max(1, static_cast<int>(std::ceil(std::abs(sweep) / perSide)));
    for (int i = 0; i < count; ++i) {
        const double a0 = from + sweep * i / count;
        const double a1 = from + sweep * (i + 1) / count;
        appendTriangle(center, onCircle(center, radius, a0), onCircle(center, radius, a1), out);
    }
}

// Quadrilatère à bouts plats, demi-largeur `half` (a et b distincts).
void appendFlatQuad(core::PointF a, core::PointF b, double half, std::vector<Vertex2D>& out)
{
    const double length = distance(a, b);
    const double nx = -(b.y - a.y) / length * half;
    const double ny = (b.x - a.x) / length * half;
    const core::PointF a0{a.x + nx, a.y + ny};
    const core::PointF a1{a.x - nx, a.y - ny};
    const core::PointF b0{b.x + nx, b.y + ny};
    const core::PointF b1{b.x - nx, b.y - ny};
    appendTriangle(a0, a1, b0, out);
    appendTriangle(b0, a1, b1, out);
}

// Jonction ronde en b entre [a, b] et [b, c] : secteur entre les normales des deux segments, des deux côtés
// (l'un comble le creux extérieur, l'autre tombe dans le trait). Rien si la direction ne change pas.
void appendRoundJoin(core::PointF a, core::PointF b, core::PointF c, double half, int sides,
                     std::vector<Vertex2D>& out)
{
    const double incoming = std::atan2(b.y - a.y, b.x - a.x);
    const double turn = wrappedTurn(std::atan2(c.y - b.y, c.x - b.x) - incoming);
    if (std::abs(turn) < 1e-9) {
        return;
    }
    appendSector(b, half, incoming + std::numbers::pi / 2, turn, sides, out);
    appendSector(b, half, incoming - std::numbers::pi / 2, turn, sides, out);
}

// Direction de a vers b ; vide si les points sont confondus.
std::optional<double> directionOf(core::PointF a, core::PointF b)
{
    if (distance(a, b) <= 1e-9) {
        return std::nullopt;
    }
    return std::atan2(b.y - a.y, b.x - a.x);
}

// Jonction ronde de taille fixe (6 sommets) en `center`, de la direction `from` à la direction `to` : un
// triangle de chaque côté entre les deux normales. Dégénérée si une direction manque.
void appendFixedJoin(core::PointF center, std::optional<double> from, std::optional<double> to, double half,
                     std::vector<Vertex2D>& out)
{
    if (!from || !to) {
        out.insert(out.end(), 6, Vertex2D{f(center.x), f(center.y)});
        return;
    }
    const double turn = wrappedTurn(*to - *from);
    for (const double side : {std::numbers::pi / 2, -std::numbers::pi / 2}) {
        appendTriangle(center, onCircle(center, half, *from + side),
                       onCircle(center, half, *from + side + turn), out);
    }
}

bool isDot(std::span<const core::StrokePoint> points)
{
    return std::all_of(points.begin(), points.end(), [&points](const core::StrokePoint& p) {
        return p.x == points.front().x && p.y == points.front().y;
    });
}

} // namespace

bool hasRoundShape(const core::Stroke& stroke) noexcept
{
    return stroke.cap != core::PenCap::Square;
}

std::vector<core::PointF> smoothedStrokePoints(std::span<const core::StrokePoint> points, double step)
{
    std::vector<core::PointF> out;
    if (points.empty()) {
        return out;
    }
    const double pace = step > 0 ? step : kSmoothingStep;
    out.reserve(points.size() * 2);
    out.push_back(pointOf(points.front()));
    if (points.size() < 3) {
        for (std::size_t i = 1; i < points.size(); ++i) {
            out.push_back(pointOf(points[i]));
        }
        return out;
    }
    core::PointF start = pointOf(points.front());
    for (std::size_t i = 1; i + 1 < points.size(); ++i) {
        const core::PointF end{(points[i].x + points[i + 1].x) / 2, (points[i].y + points[i + 1].y) / 2};
        appendQuadratic(start, pointOf(points[i]), end, pace, out);
        start = end;
    }
    out.push_back(pointOf(points.back()));
    return out;
}

QPainterPath smoothedStrokePath(std::span<const core::StrokePoint> points)
{
    QPainterPath path;
    if (points.empty()) {
        return path;
    }
    path.moveTo(points.front().x, points.front().y);
    if (points.size() < 3) {
        for (std::size_t i = 1; i < points.size(); ++i) {
            path.lineTo(points[i].x, points[i].y);
        }
        return path;
    }
    for (std::size_t i = 1; i + 1 < points.size(); ++i) {
        path.quadTo(points[i].x, points[i].y, (points[i].x + points[i + 1].x) / 2,
                    (points[i].y + points[i + 1].y) / 2);
    }
    path.lineTo(points.back().x, points.back().y);
    return path;
}

std::size_t appendRoundStrokeTriangles(std::span<const core::StrokePoint> points, double baseWidth,
                                       std::vector<Vertex2D>& out)
{
    const std::size_t before = out.size();
    const double half = baseWidth / 2;
    if (points.empty() || !(half > 0)) {
        return 0;
    }
    // Sous-segments nuls retirés : sans direction, ni quadrilatère ni jonction.
    std::vector<core::PointF> line;
    for (const core::PointF& p : smoothedStrokePoints(points)) {
        if (line.empty() || distance(line.back(), p) > 1e-6) {
            line.push_back(p);
        }
    }
    const int sides = circleSides(half);
    appendSector(line.front(), half, 0, 2 * std::numbers::pi, sides, out);
    for (std::size_t i = 0; i + 1 < line.size(); ++i) {
        appendFlatQuad(line[i], line[i + 1], half, out);
        if (i + 2 < line.size()) {
            appendRoundJoin(line[i], line[i + 1], line[i + 2], half, sides, out);
        }
    }
    if (line.size() > 1) {
        appendSector(line.back(), half, 0, 2 * std::numbers::pi, sides, out);
    }
    return out.size() - before;
}

void appendRoundInkSegment(core::PointF a, core::PointF b, const core::PointF* next, bool fromStart,
                           double baseWidth, std::vector<Vertex2D>& out)
{
    const double half = baseWidth / 2;
    const core::PointF start = fromStart ? a : core::PointF{(a.x + b.x) / 2, (a.y + b.y) / 2};
    const core::PointF end = next ? core::PointF{(b.x + next->x) / 2, (b.y + next->y) / 2} : b;
    std::array<core::PointF, kInkSubsegments + 1> at{};
    for (std::size_t i = 0; i <= kInkSubsegments; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(kInkSubsegments);
        const double u = 1 - t;
        at[i] = next ? core::PointF{u * u * start.x + 2 * u * t * b.x + t * t * end.x,
                                    u * u * start.y + 2 * u * t * b.y + t * t * end.y}
                     : core::PointF{start.x + (b.x - start.x) * t, start.y + (b.y - start.y) * t};
    }
    // Tangentes de la courbe : b − départ au départ ; arrivée − b au bout d'un arc, b − départ au bout d'une
    // droite. Celle d'arrivée d'un segment final est celle de départ du segment suivant.
    std::optional<double> previous = directionOf(start, b);
    for (std::size_t i = 0; i < kInkSubsegments; ++i) {
        const std::optional<double> chord = directionOf(at[i], at[i + 1]);
        appendFixedJoin(at[i], previous, chord, half, out);
        if (chord) {
            appendFlatQuad(at[i], at[i + 1], half, out);
            previous = chord;
        } else {
            out.insert(out.end(), 6, Vertex2D{f(at[i].x), f(at[i].y)});
        }
    }
    appendFixedJoin(at[kInkSubsegments], previous, next ? directionOf(b, end) : directionOf(start, b), half,
                    out);
}

void appendRoundInkCap(core::PointF center, double baseWidth, std::vector<Vertex2D>& out)
{
    const double half = baseWidth / 2;
    for (std::size_t i = 0; i < kInkCapSides; ++i) {
        const double a0 = 2 * std::numbers::pi * static_cast<double>(i) / static_cast<double>(kInkCapSides);
        const double a1 =
            2 * std::numbers::pi * static_cast<double>(i + 1) / static_cast<double>(kInkCapSides);
        appendTriangle(center, onCircle(center, half, a0), onCircle(center, half, a1), out);
    }
}

void paintStrokePath(QPainter& painter, const core::Stroke& stroke, const QColor& color)
{
    if (stroke.points.empty()) {
        return;
    }
    const bool rounded = hasRoundShape(stroke);
    QPen pen(color);
    pen.setWidthF(stroke.baseWidth);
    pen.setCapStyle(rounded ? Qt::RoundCap : Qt::SquareCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    const std::span<const core::StrokePoint> points(stroke.points);
    if (isDot(points)) {
        painter.drawPoint(QPointF(points.front().x, points.front().y));
        return;
    }
    if (rounded) {
        painter.drawPath(smoothedStrokePath(points));
        return;
    }
    QPainterPath path;
    path.moveTo(points.front().x, points.front().y);
    for (std::size_t i = 1; i < points.size(); ++i) {
        path.lineTo(points[i].x, points[i].y);
    }
    painter.drawPath(path);
}

std::size_t appendStrokeTriangles(std::span<const core::StrokePoint> points, double baseWidth,
                                  std::vector<Vertex2D>& out)
{
    const std::size_t before = out.size();
    for (std::size_t i = 0; i + 1 < points.size(); ++i) {
        appendQuad({points[i].x, points[i].y}, {points[i + 1].x, points[i + 1].y},
                   core::strokeWidthAt(baseWidth, points[i].pressure), out);
    }
    return out.size() - before;
}

std::size_t appendOutlineTriangles(std::span<const core::PointF> polygon, double width,
                                   std::vector<Vertex2D>& out)
{
    const std::size_t before = out.size();
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        appendQuad(polygon[i], polygon[(i + 1) % polygon.size()], width, out);
    }
    return out.size() - before;
}

std::size_t appendConvexFill(std::span<const core::PointF> polygon, std::vector<Vertex2D>& out)
{
    const std::size_t before = out.size();
    for (std::size_t i = 1; i + 1 < polygon.size(); ++i) {
        out.push_back({f(polygon[0].x), f(polygon[0].y)});
        out.push_back({f(polygon[i].x), f(polygon[i].y)});
        out.push_back({f(polygon[i + 1].x), f(polygon[i + 1].y)});
    }
    return out.size() - before;
}

std::vector<core::PointF> rectanglePolygon(const core::RectF& r)
{
    return {{r.x, r.y}, {r.x + r.width, r.y}, {r.x + r.width, r.y + r.height}, {r.x, r.y + r.height}};
}

std::vector<core::PointF> ellipsePolygon(const core::RectF& r, int segments)
{
    std::vector<core::PointF> out;
    out.reserve(static_cast<std::size_t>(segments));
    const double cx = r.x + r.width / 2;
    const double cy = r.y + r.height / 2;
    for (int i = 0; i < segments; ++i) {
        const double angle = 2 * std::numbers::pi * i / segments;
        out.push_back({cx + r.width / 2 * std::cos(angle), cy + r.height / 2 * std::sin(angle)});
    }
    return out;
}

core::RectF strokeBounds(std::span<const core::StrokePoint> points, double baseWidth)
{
    if (points.empty()) {
        return {};
    }
    double minX = points[0].x;
    double maxX = points[0].x;
    double minY = points[0].y;
    double maxY = points[0].y;
    double half = 0;
    for (const auto& p : points) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
        half = std::max(half, core::strokeWidthAt(baseWidth, p.pressure) / 2);
    }
    return {minX - half, minY - half, maxX - minX + 2 * half, maxY - minY + 2 * half};
}

} // namespace actcraft::render
