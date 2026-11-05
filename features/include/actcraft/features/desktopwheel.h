#pragma once

#include <QPointF>
#include <QString>

#include <vector>

namespace actcraft::features {

// Positions de `count` entrées sur un cercle de centre (0, 0), sens horaire depuis le haut (y vers le bas).
std::vector<QPointF> wheelPositions(int count, double radius);

// Entrée de la roue des outils de bureau (F-080) : une commande nommée, ou un anneau secondaire.
struct WheelEntry
{
    QString command; // vide pour un anneau secondaire
    QString label;
    std::vector<WheelEntry> ring;
};

std::vector<WheelEntry> defaultWheel();

} // namespace actcraft::features
