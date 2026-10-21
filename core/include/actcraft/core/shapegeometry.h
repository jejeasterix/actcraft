#pragma once

#include <actcraft/core/model.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace actcraft::core {

inline constexpr double kShapeMinDragScreen = 3.0; // en deçà, un tracé est un clic : aucune forme (F-241)

enum class ShapeFamily : std::uint8_t
{
    Line,   // objet core::Line (ligne, ligne brisée, zigzag, chevron)
    Closed, // objet core::Shape (polygone ou chemin rempli)
};

struct ShapePreset
{
    std::string id; // « rectangle »… (contrat du plan)
    ShapeFamily family = ShapeFamily::Closed;
    bool square = false;          // carré, cercle : boîte carrée
    int axis = 0;                 // 1 : horizontale seulement ; 2 : verticale seulement ; 0 : libre
    std::vector<double> defaults; // paramètres par défaut des points intelligents
    std::vector<double> minimum;  // bornes, même taille que defaults
    std::vector<double> maximum;
};

struct ShapeStyle
{
    Argb outline = 0xFF000000u;
    double width = 4;   // F-241
    bool filled = true; // E1, D2
    Argb fill = 0xFFFFFFFFu;
};

// Ordre de la palette (D1) ; le premier est le défaut.
const std::vector<ShapePreset>& shapeCatalog();
const ShapePreset* findShapePreset(std::string_view id) noexcept;

// Boîte tracée de `press` à `current` (coordonnées de page) : depuis le coin, ou depuis le centre (`press` au
// milieu) ; `square` : côté = le plus grand des deux écarts, dans le sens du geste. Toujours normalisée.
RectF dragBox(PointF press, PointF current, bool fromCenter, bool square) noexcept;

// Extrémités d'une ligne tracée : axe fixé par le préréglage, `constrain45` (Maj) arrondit l'angle à 45°.
std::pair<PointF, PointF> dragLine(PointF press, PointF current, const ShapePreset& preset, bool fromCenter,
                                   bool constrain45) noexcept;

// Objet neuf (sans identifiant, couche intermédiaire, nom vide) : Shape (preset, parameters = defaults,
// géométrie construite) ou Line, selon la famille. Préréglage inconnu : rectangle.
PageObject makeShapeObject(std::string_view preset, const RectF& box, const ShapeStyle& style);
PageObject makeLineObject(std::string_view preset, PointF from, PointF to, const ShapeStyle& style);

// Reconstruit `polygon` ou `paths` depuis preset, parameters et bounds. Faux (forme inchangée) si le
// préréglage est inconnu ou vide (forme lue).
bool rebuildShape(Shape& shape);

// Points intelligents (F-242) en coordonnées de l'objet ; vide pour une forme sans préréglage ou sans
// paramètre.
std::vector<PointF> smartPoints(const Shape& shape);

// Paramètres quand le point `index` est amené en `objectPoint` (coordonnées de l'objet), bornés par le
// préréglage.
std::vector<double> dragSmartPoint(const Shape& shape, std::size_t index, PointF objectPoint);

} // namespace actcraft::core
