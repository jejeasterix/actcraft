#pragma once

#include <actcraft/core/model.h>

class QPainter;

namespace actcraft::render {
// Dessin d'une équation (F-301). Socle : le repère neutre de paintUnknown sur equation.bounds. Rempli par
// L7-T25, qui ne change que equationlayout.cpp.
void paintEquation(QPainter& painter, const core::Equation& equation);
} // namespace actcraft::render
