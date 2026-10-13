#pragma once

#include <actcraft/render/strokegeometry.h>

#include <QColor>
#include <QSGGeometryNode>

#include <vector>

namespace actcraft::render {

// Nœud de triangles à couleur unie (chemin RHI). Possède sa géométrie et son matériau.
QSGGeometryNode* makeTrianglesNode(const std::vector<Vertex2D>& vertices, QColor color);

} // namespace actcraft::render
