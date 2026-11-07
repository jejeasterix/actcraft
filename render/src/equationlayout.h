#pragma once

#include <actcraft/core/equation.h>
#include <actcraft/render/equationpaint.h>

#include <QFont>
#include <QPainterPath>

#include <vector>

namespace actcraft::render {

// Boîte d'un nœud mis en page (F-301), en pixels : origine au début de la ligne de base, y vers le bas. `x`
// et `baseline` placent la boîte dans celle de son parent (ligne de base du parent à 0) ; la racine est à (0,
// 0).
struct EquationBox
{
    double width = 0;
    double ascent = 0;  // au-dessus de la ligne de base
    double descent = 0; // au-dessous
    double x = 0;
    double baseline = 0;
    QPainterPath ink; // glyphes et filets, coordonnées de la boîte, remplis de la couleur de l'équation
    bool placeholder = false; // case vide (marque actcraft-placeholder) : cadre pointillé en mode Editor
    bool cursor = false;      // marque actcraft-cursor : surlignée en mode Editor
    std::vector<EquationBox> children;
};

// Police de base d'une équation de `pointSize` points à 96 ppp : police du système, taille en pixels.
QFont equationFont(double pointSize);

// Met en page l'arbre à la taille de `font` (pixels ; points à 96 ppp si la taille est en points).
EquationBox layoutEquation(const core::EquationNode& node, const QFont& font);

} // namespace actcraft::render
