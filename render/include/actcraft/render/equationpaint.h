#pragma once

#include <actcraft/core/model.h>

class QPainter;

namespace actcraft::render {

// Dessin d'une équation (F-301). Page : le rendu de la page et de l'impression. Editor : en plus, les cases
// vides et le curseur de l'éditeur (marques actcraft-placeholder et actcraft-cursor du MathML).
enum class EquationPaintMode
{
    Page,
    Editor
};

void paintEquation(QPainter& painter, const core::Equation& equation,
                   EquationPaintMode mode = EquationPaintMode::Page);

} // namespace actcraft::render
