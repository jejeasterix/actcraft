#pragma once

#include <actcraft/core/model.h>

#include <cstddef>
#include <span>
#include <vector>

namespace actcraft::render {

// Même disposition que QSGGeometry::Point2D.
struct Vertex2D
{
    float x;
    float y;
};

// Chaque segment donne toujours 6 sommets (2 triangles), même nul : les blocs d'encre ont une taille fixe.
inline constexpr std::size_t kVerticesPerSegment = 6;

// Quadrilatère par segment, largeur strokeWidthAt(baseWidth, pression du premier point), bouts carrés.
// Retourne le nombre de sommets ajoutés.
std::size_t appendStrokeTriangles(std::span<const core::StrokePoint> points, double baseWidth,
                                  std::vector<Vertex2D>& out);
// Contour fermé d'un polygone, largeur fixe.
std::size_t appendOutlineTriangles(std::span<const core::PointF> polygon, double width,
                                   std::vector<Vertex2D>& out);
// Remplissage d'un polygone convexe (éventail).
std::size_t appendConvexFill(std::span<const core::PointF> polygon, std::vector<Vertex2D>& out);

std::vector<core::PointF> rectanglePolygon(const core::RectF& bounds);
std::vector<core::PointF> ellipsePolygon(const core::RectF& bounds, int segments);
// Rectangle englobant, agrandi de la demi-largeur maximale.
core::RectF strokeBounds(std::span<const core::StrokePoint> points, double baseWidth);

} // namespace actcraft::render
