#pragma once

#include <QTextDocument>

#include <cmath>

namespace actcraft::ui {

// Largeur naturelle d'un document de texte, comme textediting.cpp (measuredBounds : marge 0, ceil(idealWidth)
// + 1) : la mesure partagée par l'exécuteur d'actions et les fenêtres de lien.
inline qreal naturalTextWidth(QTextDocument& measure)
{
    measure.setTextWidth(-1);
    return std::ceil(measure.idealWidth()) + 1;
}

} // namespace actcraft::ui
