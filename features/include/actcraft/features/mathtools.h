#pragma once

#include <actcraft/features/strokeguide.h>

#include <QLineF>
#include <QPointF>
#include <QSizeF>
#include <QString>

#include <cstddef>
#include <map>
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
    // Degrés entiers, 0 à 359, en sens trigonométrique comme la référence : -angleDegrees() ramené dans
    // [0, 360) (y vers le bas : un angle positif tourne dans le sens des aiguilles d'une montre à l'écran).
    int displayAngle() const noexcept;
    void setUnits(RulerUnit top, RulerUnit bottom) noexcept;
    RulerUnit topUnit() const noexcept;
    RulerUnit bottomUnit() const noexcept;
    std::optional<input::Sample> project(const input::Sample& sample, double tolerance) const override;
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
    int displayAngle() const noexcept; // comme Ruler::displayAngle()
    std::optional<input::Sample> project(const input::Sample& sample, double tolerance) const override;
    std::optional<input::Sample> capture(const input::Sample& sample, double tolerance) override;
    input::Sample follow(const input::Sample& sample) override;

private:
    QPointF m_vertex{0, 0};
    double m_side = 300;
    double m_angle = 0;
    int m_edge = -1;
};

enum class ArcStyle
{
    Arc,
    Closed,
    Filled
};
// Points d'un arc, de startDegrees à startDegrees + sweepDegrees inclus, pas de 2° (au moins un pas, comptés
// sur |sweepDegrees|) ; angles en degrés dans le repère de page (y vers le bas). Closed et Filled : le centre
// en premier et en dernier (deux rayons) ; Filled se remplit tel quel.
std::vector<QPointF> arcPath(QPointF center, double radius, double startDegrees, double sweepDegrees,
                             ArcStyle style);

// Rapporteur (F-281) : le pourtour guide le stylo ; demi : seulement le demi-cercle « au-dessus » du diamètre
// (y local ≤ 0 à l'angle 0).
class Protractor final : public StrokeGuide
{
public:
    enum class Kind
    {
        Full,
        Half
    };
    void setFrame(QPointF center, double radius, double angleDegrees, Kind kind);
    int displayAngle() const noexcept;    // comme Ruler::displayAngle()
    double sweptDegrees() const noexcept; // valeur absolue de l'angle cumulé depuis la saisie
    // Angle cumulé avec son sens, dans le repère de page : négatif vers le haut de l'écran (sens
    // trigonométrique). arcPath(centre, rayon, startDegrees(), signedSweptDegrees(), …) suit le trait.
    double signedSweptDegrees() const noexcept;
    double startDegrees() const noexcept; // angle de page du point saisi (repère d'arcPath)
    std::optional<input::Sample> project(const input::Sample& sample, double tolerance) const override;
    std::optional<input::Sample> capture(const input::Sample& sample, double tolerance) override;
    input::Sample follow(const input::Sample& sample) override;

private:
    // Angle de page du point du pourtour sous `sample`, si `sample` est à moins de `tolerance` du pourtour
    // (demi : pas sous le diamètre) ; requête pure.
    std::optional<double> rimAngle(const input::Sample& sample, double tolerance) const;

    QPointF m_center{0, 0};
    double m_radius = 200;
    double m_angle = 0;
    Kind m_kind = Kind::Full;
    double m_startDegrees = 0;
    double m_lastDegrees = 0;
    double m_swept = 0;
};

// Compas (F-282) : la mine (pointe + rayon à l'angle courant) saisit le stylo ; le trait suit le cercle de
// rayon courant ; au-delà d'un tour, il se ferme exactement sur son premier point.
class Compass final : public StrokeGuide
{
public:
    static constexpr double kMinRadius = 20;
    static constexpr double kMaxRadius = 2000;
    void setPivot(QPointF point) noexcept;
    void setRadius(double pixels) noexcept; // borné à [kMinRadius, kMaxRadius]
    void setAngle(double degrees) noexcept;
    void setMirrored(bool on) noexcept; // dessin retourné (délégué QML, phase 2) ; la mine ne bouge pas
    bool mirrored() const noexcept;
    QPointF pivot() const noexcept;
    double radius() const noexcept;
    QPointF pencil() const noexcept;      // position de la mine
    int displayAngle() const noexcept;    // angle du bras, comme Ruler::displayAngle()
    double sweptDegrees() const noexcept; // valeur absolue de l'angle cumulé
    bool closedCircle() const noexcept;   // balayage ≥ 360°
    QString radiusText() const;           // « 3,0 cm » : une décimale, virgule française
    std::optional<input::Sample> project(const input::Sample& sample, double tolerance) const override;
    std::optional<input::Sample> capture(const input::Sample& sample, double tolerance) override;
    input::Sample follow(const input::Sample& sample) override;

private:
    // Angle de page du point du cercle sous `sample`, si `sample` est à moins de `tolerance` de la mine ;
    // requête pure.
    std::optional<double> pencilAngle(const input::Sample& sample, double tolerance) const;

    QPointF m_pivot{0, 0};
    double m_radius = 150;
    double m_angle = 0;
    bool m_mirrored = false;
    double m_lastDegrees = 0;
    double m_swept = 0;
    QPointF m_first;
};

// Origine XY (F-284) par page ; défaut : centre de la page. La phase 2 la remplit depuis le modèle de la
// page.
class OriginMemory
{
public:
    explicit OriginMemory(QSizeF pageSize);
    void setPageSize(QSizeF pageSize) noexcept;
    void set(std::size_t page, QPointF origin);
    void forget(std::size_t page);
    bool has(std::size_t page) const;
    QPointF at(std::size_t page) const;

private:
    QSizeF m_size; // taille de la page (nom court : la forme longue est refusée par le contrôle des noms)
    std::map<std::size_t, QPointF> m_origins;
};

} // namespace actcraft::features
