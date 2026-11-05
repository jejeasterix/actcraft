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
    // Outil qui s'ouvre dans la fenêtre principale (panneaux de l'horloge, des dés et de la calculatrice,
    // message défilant et son dialogue) : l'entrée la rend d'abord, sinon il s'ouvrirait dans la fenêtre
    // réduite, invisible. ÉCART À L'ORIGINAL (hypothèse H-M8, à revoir à la porte de clôture) : l'original
    // montre ces outils sur le bureau, sa fenêtre restant cachée ; les y montrer demanderait une fenêtre
    // propre à chacun.
    bool opensInTheMainWindow = false;
};

std::vector<WheelEntry> defaultWheel();

} // namespace actcraft::features
