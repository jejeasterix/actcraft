#include <actcraft/features/mathtools.h>

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
std::optional<input::Sample> snapToEdge(const std::vector<QLineF>& edges, const input::Sample& sample,
                                        double tolerance, int* seized = nullptr)
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
    return snapToEdge(edges(), sample, tolerance);
}

std::optional<input::Sample> Ruler::capture(const input::Sample& sample, double tolerance)
{
    return snapToEdge(edges(), sample, tolerance, &m_edge);
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
    return snapToEdge(edges(), sample, tolerance);
}

std::optional<input::Sample> SetSquare::capture(const input::Sample& sample, double tolerance)
{
    return snapToEdge(edges(), sample, tolerance, &m_edge);
}

input::Sample SetSquare::follow(const input::Sample& sample)
{
    return followEdge(edges(), m_edge, sample);
}

} // namespace actcraft::features
