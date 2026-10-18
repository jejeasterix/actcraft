#pragma once

#include <QImage>
#include <QPointF>
#include <QPolygonF>
#include <QRect>
#include <QRectF>

namespace actcraft::features {

inline constexpr double kClosePixels = 10; // un clic à moins de 10 px du premier point ferme le polygone

// Capture point par point (F-275) : chaque clic ajoute un sommet ; un clic près du premier (3 sommets au moins) ferme.
class PolygonPicker
{
public:
    void click(QPointF point);
    bool closed() const noexcept;
    QPolygonF polygon() const; // sommets, sans le clic de fermeture
    void clear();

private:
    QPolygonF m_points;
    bool m_closed = false;
};

// Capture à main levée : le relâché ferme par une droite (3 points au moins).
class FreehandPicker
{
public:
    void press(QPointF point);
    void move(QPointF point);
    void release();
    bool closed() const noexcept;
    QPolygonF polygon() const; // fermé : dernier point = premier
    void clear();

private:
    QPolygonF m_points;
    bool m_closed = false;
};

// Image de la boîte du polygone (bornée à l'image) ; hors du polygone : transparent.
QImage cropToPolygon(const QImage& screen, const QPolygonF& polygon);
QRectF normalizedRect(QPointF a, QPointF b);
// Rectangle en pixels physiques (DwmGetWindowAttribute) ramené en pixels logiques ; rapport ≤ 0 : 1.
QRectF toLogicalPixels(const QRect& physical, qreal devicePixelRatio);

} // namespace actcraft::features
