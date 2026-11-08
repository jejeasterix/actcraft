#pragma once

#include <QPointF>
#include <QRectF>
#include <QSizeF>

namespace actcraft::features {

// Store du rideau (F-265, F-592) dans un rectangle de taille `area` : quatre bords mobiles, le store couvre
// ce qui reste entre eux. Un appui à moins de kGrabMargin d'un bord du store (ou du cadre, store fermé)
// saisit ce bord.
class Curtain
{
public:
    enum class Start
    {
        Full,
        Top,
        Bottom,
        Left,
        Right
    };
    static constexpr double kGrabMargin = 24; // pixels de la zone (unités de `area`)

    explicit Curtain(QSizeF area);
    void setArea(QSizeF area);                // garde les proportions découvertes
    void reset(Start start, double distance); // store fermé, ouvert de `distance` depuis le bord `start`
    bool press(QPointF point);                // vrai si un bord est saisi
    void move(QPointF point);
    void release();
    bool dragging() const noexcept;
    QRectF covered() const noexcept;  // vide : tout découvert
    double coverage() const noexcept; // aire couverte / aire de la zone, 0 à 1

private:
    enum class Edge
    {
        None,
        Top,
        Bottom,
        Left,
        Right
    };
    QSizeF m_area;
    double m_top = 0, m_bottom = 0, m_left = 0,
           m_right = 0; // fractions découvertes depuis chaque bord, 0 à 1
    Edge m_edge = Edge::None;
};

} // namespace actcraft::features
