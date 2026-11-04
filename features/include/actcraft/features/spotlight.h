#pragma once

#include <QPointF>
#include <QRectF>
#include <QSizeF>

namespace actcraft::features {

// Spot (F-270) dans un rectangle de taille `area` : un spot montre le cadre et masque le reste ; un cache
// plein masque le cadre. Le pourtour du cadre (à kRimMargin près) le redimensionne côté par côté (ovale,
// rectangle) ; la partie masquée le déplace ; ailleurs, l'appui n'est pas pris (il traverse vers la page).
class Spotlight
{
public:
    // Mêmes valeurs que core::SpotlightShape (socle L0-S3) : toPageToolsSettings convertit de l'une à
    // l'autre, valeurs vérifiées par les static_assert de pagetools.cpp.
    enum class Shape
    {
        Off = 0,
        Circle = 1,
        Square = 2,
        SolidCircle = 3,
        SolidSquare = 4
    };
    static constexpr double kDefaultSize = 300; // côté du cadre par défaut, unités de `area`
    static constexpr double kMinimumSize = 40;  // un cadre plus petit ne se saisit plus au doigt
    static constexpr double kRimMargin = 16;    // largeur de la bande de saisie du pourtour
    // Largeur du contour dessiné autour du cadre (lue par la surcouche, phase 2) : 10, réglage embarqué
    // relevé par L7-T0 (docs/verifications/2026-10-18-lot7-principes.md ; mesuré 9 ; la spécification disait
    // 15).
    static constexpr double kDefaultOutline = 10;

    explicit Spotlight(QSizeF area);
    void setArea(QSizeF area);
    void setShape(Shape shape);
    Shape shape() const noexcept;
    void setFrame(QRectF frame); // vide : cadre par défaut, centré, kDefaultSize de côté
    QRectF frame() const noexcept;
    bool hidesInside() const noexcept; // vrai pour SolidCircle et SolidSquare
    bool press(QPointF point);         // vrai si le cadre est saisi (déplacer ou redimensionner)
    void move(QPointF point);
    void release();
    bool dragging() const noexcept;

private:
    enum class Grab
    {
        None,
        Move,
        Left,
        Right,
        Top,
        Bottom
    };
    QRectF defaultFrame() const;
    void keepInside(); // le centre du cadre reste dans la zone : le cadre garde une intersection avec elle

    QSizeF m_area;
    Shape m_shape = Shape::Off;
    QRectF m_frame;
    Grab m_grab = Grab::None;
    QPointF m_last;
};

} // namespace actcraft::features
