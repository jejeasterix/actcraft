// Talon L0-S : rempli par L7-T25.
#include "equationlayout.h"

#include <QColor>
#include <QPainter>
#include <QPen>
#include <QRectF>

namespace actcraft::render {

// Repère neutre de paintUnknown (pagepainter.cpp) sur la boîte de l'équation : cadre gris pointillé d'un
// pixel quel que soit le zoom (plume cosmétique). Le MathML n'est pas lu.
void paintEquation(QPainter& painter, const core::Equation& equation)
{
    QPen pen(QColor(0x80, 0x80, 0x80), 0, Qt::DashLine);
    pen.setCosmetic(true);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    const core::RectF& b = equation.bounds;
    painter.drawRect(QRectF(b.x, b.y, b.width, b.height));
}

} // namespace actcraft::render
