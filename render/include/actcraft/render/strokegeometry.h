#pragma once

#include <actcraft/core/model.h>

#include <cstddef>
#include <span>
#include <vector>

class QColor;
class QPainter;
class QPainterPath;

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

// Forme ronde (D2) : bouts et jonctions ronds, courbe lissée par les milieux, largeur baseWidth constante.
// Tout trait sauf le bout carré du moteur du prototype : le bout écrit dans un fichier est ignoré (un trait
// lu prend la forme d'un trait dessiné).
bool hasRoundShape(const core::Stroke& stroke) noexcept;
// Pas d'échantillonnage de la courbe lissée, en unités de page.
inline constexpr double kSmoothingStep = 2.0;
// Courbe par les milieux (D2, comme l'encre de l'ancien dépôt) : départ au premier point, courbe quadratique
// de milieu en milieu (points de contrôle aux points d'origine), arrivée au dernier point. Points :
// échantillonnée à pas fixe (`step` unités de page au plus par sous-segment) ; chemin : moveTo, quadTo,
// lineTo. Deux points ou moins : les points tels quels.
std::vector<core::PointF> smoothedStrokePoints(std::span<const core::StrokePoint> points,
                                               double step = kSmoothingStep);
QPainterPath smoothedStrokePath(std::span<const core::StrokePoint> points);
// Trait rond, largeur baseWidth : courbe lissée, un quadrilatère à bouts plats par sous-segment, disque plein
// à chaque bout, secteur des deux côtés de chaque jonction. Un point seul : un disque. Les triangles se
// recouvrent : géométrie réservée au trait opaque. Un trait translucide passe par le moteur peint
// (usesPaintedRenderer), d'un seul drawPath : une couleur à alpha sur ces triangles foncerait à chaque
// jonction et à chaque croisement (D3).
std::size_t appendRoundStrokeTriangles(std::span<const core::StrokePoint> points, double baseWidth,
                                       std::vector<Vertex2D>& out);
// Trait d'un seul chemin (peintre logiciel, D3 : un trait translucide ne fonce pas sur lui-même) : plume de
// largeur baseWidth, jonctions rondes ; forme ronde : bouts ronds et courbe lissée, sinon bouts carrés et
// ligne brisée. Un point, ou des points confondus : un point de la plume.
void paintStrokePath(QPainter& painter, const core::Stroke& stroke, const QColor& color);

// Encre vive à forme ronde (D2), en blocs de taille fixe (InkItem) : un segment du trait (deux points
// successifs) donne toujours kRoundInkVerticesPerSegment sommets, un bout rond kRoundInkCapVertices.
inline constexpr std::size_t kInkSubsegments = 4;
inline constexpr std::size_t kInkCapSides = 16;
// Par sous-segment : jonction à son départ (un triangle de chaque côté) et quadrilatère ; plus la jonction
// d'arrivée.
inline constexpr std::size_t kRoundInkVerticesPerSegment = kInkSubsegments * 12 + 6;
inline constexpr std::size_t kRoundInkCapVertices = kInkCapSides * 3;
// Part de la courbe par les milieux (même courbe que smoothedStrokePath) portée par le segment a → b, largeur
// baseWidth : départ en a si `fromStart` (premier segment du trait), sinon au milieu de [a, b] ; arrivée au
// milieu de [b, *next] par un arc de contrôle b, ou en b par une droite si `next` est nul (dernier segment,
// provisoire : réécrit quand le point suivant arrive). kInkSubsegments sous-segments à bouts plats ;
// jonctions rondes entre eux et aux deux bouts, prises sur la tangente de la courbe, continue d'un segment au
// suivant. Toujours kRoundInkVerticesPerSegment sommets (dégénérés là où rien n'est à couvrir).
void appendRoundInkSegment(core::PointF a, core::PointF b, const core::PointF* next, bool fromStart,
                           double baseWidth, std::vector<Vertex2D>& out);
// Disque d'un bout rond de l'encre vive, diamètre baseWidth : kRoundInkCapVertices sommets.
void appendRoundInkCap(core::PointF center, double baseWidth, std::vector<Vertex2D>& out);

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
