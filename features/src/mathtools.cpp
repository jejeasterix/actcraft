#include <actcraft/features/mathtools.h>

#include <QLocale>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace actcraft::features {

namespace {

// `local` tourné de `degrees` autour de l'origine du repère local, posé en `origin` (repère de page, y vers
// le bas : un angle positif tourne dans le sens des aiguilles d'une montre à l'écran).
QPointF rotated(QPointF local, QPointF origin, double degrees)
{
    const double a = qDegreesToRadians(degrees);
    const double c = std::cos(a);
    const double s = std::sin(a);
    return origin + QPointF(local.x() * c - local.y() * s, local.x() * s + local.y() * c);
}

// Degrés entiers ramenés dans 0..359 (négatifs compris).
int wholeDegrees(double degrees) noexcept
{
    return ((qRound(degrees) % 360) + 360) % 360;
}

// Projection orthogonale de `point` sur `segment`, bornée à ses extrémités.
QPointF projectOnSegment(const QLineF& segment, QPointF point)
{
    const QPointF a = segment.p1();
    const QPointF d = segment.p2() - a;
    const double lengthSquared = QPointF::dotProduct(d, d);
    if (lengthSquared <= 0) {
        return a; // segment réduit à un point
    }
    const double t = std::clamp(QPointF::dotProduct(point - a, d) / lengthSquared, 0.0, 1.0);
    return a + t * d;
}

// L'échantillon placé en `point` : seuls x et y changent (pression, instant, appareil, phase et numéro
// gardés).
input::Sample movedTo(const input::Sample& sample, QPointF point)
{
    input::Sample out = sample;
    out.x = point.x();
    out.y = point.y();
    return out;
}

// Projection sur le bord le plus proche de `edges`, s'il est à moins de `tolerance`. Avec `seized`, l'index
// de ce bord y est noté (saisie) ; sans, rien n'est noté (requête pure de project()).
std::optional<input::Sample> projectOnNearestEdge(const std::vector<QLineF>& edges,
                                                  const input::Sample& sample, double tolerance,
                                                  int* seized = nullptr)
{
    const SegmentHit hit = projectOnSegments(edges, QPointF(sample.x, sample.y));
    if (hit.index < 0 || hit.distance > tolerance) {
        return std::nullopt;
    }
    if (seized) {
        *seized = hit.index;
    }
    return movedTo(sample, hit.point);
}

// Suivi du bord saisi, et de lui seul : jamais le bord le plus proche, sinon le trait sauterait d'un bord à
// l'autre dès qu'il passe plus près du bord opposé (milieu de la règle franchi).
input::Sample followEdge(const std::vector<QLineF>& edges, int seized, const input::Sample& sample)
{
    if (seized < 0 || static_cast<std::size_t>(seized) >= edges.size()) {
        return sample;
    }
    return movedTo(sample,
                   projectOnSegment(edges[static_cast<std::size_t>(seized)], QPointF(sample.x, sample.y)));
}

// `to - from` ramené dans ]-180, 180] : le pas le plus court d'un angle au suivant. Une différence d'atan2
// brute sauterait de 360° chaque fois que le trait passe le rayon gauche (atan2 va de 180 à -180), et l'écart
// entre l'angle actuel et celui du départ oublierait les tours complets.
double unwrapDelta(double from, double to)
{
    double d = std::fmod(to - from, 360.0);
    if (d > 180) {
        d -= 360;
    } else if (d <= -180) {
        d += 360;
    }
    return d;
}

// Angle de page (degrés) de `point` vu de `center` ; rien au centre même (direction indéfinie).
std::optional<double> angleFrom(QPointF center, QPointF point)
{
    const QPointF d = point - center;
    if (d.isNull()) {
        return std::nullopt;
    }
    return qRadiansToDegrees(std::atan2(d.y(), d.x()));
}

// Point du cercle (`center`, `radius`) à l'angle de page `degrees`.
QPointF onCircle(QPointF center, double radius, double degrees)
{
    const double a = qDegreesToRadians(degrees);
    return center + radius * QPointF(std::cos(a), std::sin(a));
}

// Angle local, depuis le diamètre d'un demi-rapporteur tourné de `frameDegrees`, de l'angle de page `degrees`
// ramené sur le demi-pourtour : de 0 (bout droit) à -180 (bout gauche). Sous le diamètre, le bout le plus
// proche : le point du demi-pourtour le plus proche, comme la règle bornée à sa longueur (un clamp dans
// [-180, 0] enverrait 170, voisin du bout gauche, au bout droit).
double onUpperHalf(double frameDegrees, double degrees)
{
    const double local = unwrapDelta(frameDegrees, degrees); // ]-180, 180]
    if (local <= 0) {
        return local;
    }
    return local <= 90 ? 0.0 : -180.0; // 180 : le bout gauche lui-même
}

} // namespace

SegmentHit projectOnSegments(const std::vector<QLineF>& segments, QPointF point)
{
    SegmentHit hit;
    for (std::size_t i = 0; i < segments.size(); ++i) {
        const QPointF projected = projectOnSegment(segments[i], point);
        const double d = std::hypot(projected.x() - point.x(), projected.y() - point.y());
        if (hit.index < 0 || d < hit.distance) {
            hit.index = static_cast<int>(i);
            hit.point = projected;
            hit.distance = d;
        }
    }
    return hit;
}

// --- Règle
// ---------------------------------------------------------------------------------------------------

void Ruler::setFrame(QPointF center, double length, double width, double angleDegrees)
{
    m_center = center;
    m_length = length;
    m_width = width;
    m_angle = angleDegrees;
}

QPointF Ruler::center() const noexcept
{
    return m_center;
}

double Ruler::length() const noexcept
{
    return m_length;
}

double Ruler::width() const noexcept
{
    return m_width;
}

double Ruler::angleDegrees() const noexcept
{
    return m_angle;
}

std::vector<QLineF> Ruler::edges() const
{
    const double halfLength = m_length / 2;
    const double halfWidth = m_width / 2;
    return {
        QLineF(rotated(QPointF(-halfLength, -halfWidth), m_center, m_angle),
               rotated(QPointF(halfLength, -halfWidth), m_center, m_angle)),
        QLineF(rotated(QPointF(-halfLength, halfWidth), m_center, m_angle),
               rotated(QPointF(halfLength, halfWidth), m_center, m_angle)),
    };
}

int Ruler::displayAngle() const noexcept
{
    return wholeDegrees(-m_angle); // sens trigonométrique à l'écran (référence)
}

void Ruler::setUnits(RulerUnit top, RulerUnit bottom) noexcept
{
    m_topUnit = top;
    m_bottomUnit = bottom;
}

RulerUnit Ruler::topUnit() const noexcept
{
    return m_topUnit;
}

RulerUnit Ruler::bottomUnit() const noexcept
{
    return m_bottomUnit;
}

std::optional<input::Sample> Ruler::project(const input::Sample& sample, double tolerance) const
{
    return projectOnNearestEdge(edges(), sample, tolerance);
}

std::optional<input::Sample> Ruler::capture(const input::Sample& sample, double tolerance)
{
    return projectOnNearestEdge(edges(), sample, tolerance, &m_edge);
}

input::Sample Ruler::follow(const input::Sample& sample)
{
    return followEdge(edges(), m_edge, sample);
}

// --- Équerre
// -------------------------------------------------------------------------------------------------

void SetSquare::setFrame(QPointF rightAngleVertex, double side, double angleDegrees)
{
    m_vertex = rightAngleVertex;
    m_side = side;
    m_angle = angleDegrees;
}

std::vector<QLineF> SetSquare::edges() const
{
    // u = direction de l'angle, n = u tourné de +90° : extrémités des deux côtés de l'angle droit.
    const QPointF alongU = rotated(QPointF(m_side, 0), m_vertex, m_angle);
    const QPointF alongN = rotated(QPointF(0, m_side), m_vertex, m_angle);
    return {QLineF(m_vertex, alongU), QLineF(m_vertex, alongN), QLineF(alongU, alongN)};
}

int SetSquare::displayAngle() const noexcept
{
    return wholeDegrees(-m_angle);
}

std::optional<input::Sample> SetSquare::project(const input::Sample& sample, double tolerance) const
{
    return projectOnNearestEdge(edges(), sample, tolerance);
}

std::optional<input::Sample> SetSquare::capture(const input::Sample& sample, double tolerance)
{
    return projectOnNearestEdge(edges(), sample, tolerance, &m_edge);
}

input::Sample SetSquare::follow(const input::Sample& sample)
{
    return followEdge(edges(), m_edge, sample);
}

// --- Arcs
// ----------------------------------------------------------------------------------------------------

std::vector<QPointF> arcPath(QPointF center, double radius, double startDegrees, double sweepDegrees,
                             ArcStyle style)
{
    const int steps = std::max(1, static_cast<int>(std::ceil(std::abs(sweepDegrees) / 2)));
    const bool radii = style != ArcStyle::Arc;
    std::vector<QPointF> path;
    path.reserve(static_cast<std::size_t>(steps) + 3);
    if (radii) {
        path.push_back(center);
    }
    for (int i = 0; i <= steps; ++i) {
        path.push_back(
            onCircle(center, radius, startDegrees + sweepDegrees * static_cast<double>(i) / steps));
    }
    if (radii) {
        path.push_back(center);
    }
    return path;
}

// --- Rapporteur
// -----------------------------------------------------------------------------------------------

void Protractor::setFrame(QPointF center, double radius, double angleDegrees, Kind kind)
{
    m_center = center;
    m_radius = radius;
    m_angle = angleDegrees;
    m_kind = kind;
}

int Protractor::displayAngle() const noexcept
{
    return wholeDegrees(-m_angle);
}

double Protractor::sweptDegrees() const noexcept
{
    return std::abs(m_swept); // le sens du trait ne compte pas : l'échelle lit un angle positif
}

double Protractor::signedSweptDegrees() const noexcept
{
    return m_swept;
}

double Protractor::startDegrees() const noexcept
{
    return m_startDegrees;
}

std::optional<double> Protractor::rimAngle(const input::Sample& sample, double tolerance) const
{
    const QPointF point(sample.x, sample.y);
    const auto degrees = angleFrom(m_center, point);
    if (!degrees) {
        return std::nullopt;
    }
    const QPointF d = point - m_center;
    if (std::abs(std::hypot(d.x(), d.y()) - m_radius) > tolerance) {
        return std::nullopt;
    }
    if (m_kind == Kind::Half) {
        const double local = unwrapDelta(m_angle, *degrees);
        if (local > 0 && local < 180) {
            return std::nullopt; // sous le diamètre
        }
    }
    return degrees;
}

std::optional<input::Sample> Protractor::project(const input::Sample& sample, double tolerance) const
{
    const auto degrees = rimAngle(sample, tolerance);
    if (!degrees) {
        return std::nullopt;
    }
    return movedTo(sample, onCircle(m_center, m_radius, *degrees));
}

std::optional<input::Sample> Protractor::capture(const input::Sample& sample, double tolerance)
{
    const auto degrees = rimAngle(sample, tolerance);
    if (!degrees) {
        return std::nullopt;
    }
    m_startDegrees = *degrees;
    m_lastDegrees = *degrees;
    m_swept = 0;
    return movedTo(sample, onCircle(m_center, m_radius, *degrees));
}

input::Sample Protractor::follow(const input::Sample& sample)
{
    // Aucune nouvelle saisie : loin du pourtour ou non, le stylo y est ramené à son angle (au centre même,
    // l'angle précédent).
    double degrees = angleFrom(m_center, QPointF(sample.x, sample.y)).value_or(m_lastDegrees);
    if (m_kind == Kind::Half) {
        // Le trait reste dans [-180, 0] en local, intervalle sans tour : la différence des angles locaux y
        // est exacte et jamais plus de 180 (unwrapDelta prendrait un demi-tour d'un seul pas, -180, pour
        // +180).
        const double local = onUpperHalf(m_angle, degrees);
        m_swept += local - onUpperHalf(m_angle, m_lastDegrees);
        degrees = m_angle + local;
    } else {
        m_swept += unwrapDelta(m_lastDegrees, degrees);
    }
    m_lastDegrees = degrees;
    return movedTo(sample, onCircle(m_center, m_radius, degrees));
}

// --- Compas
// ---------------------------------------------------------------------------------------------------

void Compass::setPivot(QPointF point) noexcept
{
    m_pivot = point;
}

void Compass::setRadius(double pixels) noexcept
{
    m_radius = std::clamp(pixels, kMinRadius, kMaxRadius);
}

void Compass::setAngle(double degrees) noexcept
{
    m_angle = degrees;
}

void Compass::setMirrored(bool on) noexcept
{
    m_mirrored = on;
}

bool Compass::mirrored() const noexcept
{
    return m_mirrored;
}

QPointF Compass::pivot() const noexcept
{
    return m_pivot;
}

double Compass::radius() const noexcept
{
    return m_radius;
}

QPointF Compass::pencil() const noexcept
{
    return onCircle(m_pivot, m_radius, m_angle);
}

int Compass::displayAngle() const noexcept
{
    return wholeDegrees(-m_angle);
}

double Compass::sweptDegrees() const noexcept
{
    return std::abs(m_swept); // un tour se ferme dans un sens comme dans l'autre
}

bool Compass::closedCircle() const noexcept
{
    return sweptDegrees() >= 360;
}

QString Compass::radiusText() const
{
    return QLocale(QLocale::French).toString(m_radius / kPixelsPerCentimetre, 'f', 1) + QStringLiteral(" cm");
}

std::optional<double> Compass::pencilAngle(const input::Sample& sample, double tolerance) const
{
    const QPointF point(sample.x, sample.y);
    const QPointF tip = pencil();
    if (std::hypot(point.x() - tip.x(), point.y() - tip.y()) > tolerance) {
        return std::nullopt; // seule la mine saisit le stylo, pas le reste du cercle
    }
    return angleFrom(m_pivot, point).value_or(m_angle);
}

std::optional<input::Sample> Compass::project(const input::Sample& sample, double tolerance) const
{
    const auto degrees = pencilAngle(sample, tolerance);
    if (!degrees) {
        return std::nullopt;
    }
    return movedTo(sample, onCircle(m_pivot, m_radius, *degrees));
}

std::optional<input::Sample> Compass::capture(const input::Sample& sample, double tolerance)
{
    const auto degrees = pencilAngle(sample, tolerance);
    if (!degrees) {
        return std::nullopt;
    }
    m_angle = *degrees; // la mine se place sous le stylo
    m_lastDegrees = *degrees;
    m_swept = 0;
    m_first = onCircle(m_pivot, m_radius, *degrees);
    return movedTo(sample, m_first);
}

input::Sample Compass::follow(const input::Sample& sample)
{
    // Aucune nouvelle saisie : loin de la mine ou du cercle, le stylo est ramené sur le cercle à son angle.
    const double degrees = angleFrom(m_pivot, QPointF(sample.x, sample.y)).value_or(m_lastDegrees);
    m_swept += unwrapDelta(m_lastDegrees, degrees);
    m_lastDegrees = degrees;
    m_angle = degrees; // la mine suit le stylo
    if (closedCircle()) {
        return movedTo(sample, m_first); // fermé exactement sur le premier point
    }
    return movedTo(sample, onCircle(m_pivot, m_radius, degrees));
}

// --- Origine XY
// -----------------------------------------------------------------------------------------------

OriginMemory::OriginMemory(QSizeF pageSize) : m_size(pageSize) {}

void OriginMemory::setPageSize(QSizeF pageSize) noexcept
{
    m_size = pageSize;
}

void OriginMemory::set(std::size_t page, QPointF origin)
{
    m_origins[page] = origin;
}

void OriginMemory::forget(std::size_t page)
{
    m_origins.erase(page);
}

bool OriginMemory::has(std::size_t page) const
{
    return m_origins.contains(page);
}

QPointF OriginMemory::at(std::size_t page) const
{
    const auto it = m_origins.find(page);
    return it != m_origins.end() ? it->second : QPointF(m_size.width() / 2, m_size.height() / 2);
}

} // namespace actcraft::features
