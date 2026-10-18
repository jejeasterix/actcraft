#pragma once

#include <actcraft/features/strokeguide.h>

#include <QLineF>
#include <QPointF>

#include <optional>
#include <vector>

namespace actcraft::features {

// Échelle des graduations : 96 pixels de page par pouce (référence de l'ancien dépôt, reformulée).
inline constexpr double kPixelsPerCentimetre = 96.0 / 2.54;
inline constexpr double kPixelsPerInch = 96.0;
// Translucidité par défaut des outils de maths, sur 255 (lue par les délégués QML, phase 2) : 100, réglage
// embarqué relevé par L7-T0 (docs/verifications/2026-10-18-lot7-principes.md), et non 255.
inline constexpr int kDefaultToolOpacity = 100;

enum class RulerUnit
{
    Cm,
    Mm,
    Inch,
    None
};

// Projection bornée de `point` sur le segment le plus proche de `segments` ; index -1 si la liste est vide.
struct SegmentHit
{
    int index = -1;
    QPointF point;
    double distance = 0;
};
SegmentHit projectOnSegments(const std::vector<QLineF>& segments, QPointF point);

// Règle (F-280) : rectangle tourné ; ses deux grands bords guident le stylo.
class Ruler final : public StrokeGuide
{
public:
    void setFrame(QPointF center, double length, double width, double angleDegrees);
    QPointF center() const noexcept;
    double length() const noexcept;
    double width() const noexcept;
    double angleDegrees() const noexcept;
    std::vector<QLineF> edges() const; // [0] bord haut, [1] bord bas (à l'angle 0)
    int displayAngle() const noexcept; // degrés entiers, 0 à 359
    void setUnits(RulerUnit top, RulerUnit bottom) noexcept;
    RulerUnit topUnit() const noexcept;
    RulerUnit bottomUnit() const noexcept;
    std::optional<input::Sample> capture(const input::Sample& sample, double tolerance) override;
    input::Sample follow(const input::Sample& sample) override;

private:
    QPointF m_center{0, 0};
    double m_length = 600; // défaut d'ouverture (7a, L7-T9)
    double m_width = 80;
    double m_angle = 0;
    RulerUnit m_topUnit = RulerUnit::Cm;
    RulerUnit m_bottomUnit = RulerUnit::Inch;
    int m_edge = -1; // bord saisi au dernier capture() réussi
};

// Équerre (F-283) : triangle rectangle isocèle ; côtés de l'angle droit vers +x et +y depuis le sommet à
// l'angle 0.
class SetSquare final : public StrokeGuide
{
public:
    void setFrame(QPointF rightAngleVertex, double side, double angleDegrees);
    std::vector<QLineF> edges() const; // [0] côté +x, [1] côté +y, [2] hypoténuse
    int displayAngle() const noexcept;
    std::optional<input::Sample> capture(const input::Sample& sample, double tolerance) override;
    input::Sample follow(const input::Sample& sample) override;

private:
    QPointF m_vertex{0, 0};
    double m_side = 300;
    double m_angle = 0;
    int m_edge = -1;
};

} // namespace actcraft::features
