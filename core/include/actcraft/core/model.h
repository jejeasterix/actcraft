#pragma once

#include <cstdint>
#include <variant>
#include <vector>

// Modèle minimal d'une page (décision 2 du cadrage du prototype de mesure). Sans QtGui : couleur en entier.

namespace actcraft::core {

// Couleur ARGB 32 bits (0xAARRGGBB), même codage que QRgb.
using Argb = std::uint32_t;

struct PointF
{
    double x = 0;
    double y = 0;
};

struct RectF
{
    double x = 0;
    double y = 0;
    double width = 0;
    double height = 0;

    bool contains(PointF p) const noexcept
    {
        return p.x >= x && p.x <= x + width && p.y >= y && p.y <= y + height;
    }
    PointF center() const noexcept { return {x + width / 2, y + height / 2}; }
};

// L'ordre suit celui de PageObject : kindOf() en dépend.
enum class ObjectKind : std::uint8_t
{
    Shape = 0,
    Image = 1,
    Stroke = 2,
};

enum class ShapeKind : std::uint8_t
{
    Rectangle = 0,
    Ellipse = 1,
    Polygon = 2,
};

struct Shape
{
    ShapeKind kind = ShapeKind::Rectangle;
    RectF bounds;
    std::vector<PointF> polygon; // sommets en coordonnées de page (Polygon seulement)
    Argb fill = 0xFFFFFFFFu;
    Argb outline = 0xFF000000u;
    double outlineWidth = 1; // 0 : pas de contour
    double opacity = 1;
};

enum class ImagePattern : std::uint8_t
{
    LinearGradient = 0,
    Checker = 1,
    Rings = 2,
    SolidCenter = 3, // moitié centrale unie (colorA), bord colorB : objet témoin
};

// Image procédurale : seuls ses paramètres sont dans le modèle (décision 4).
struct ImageObject
{
    RectF bounds;
    std::int32_t sourceWidth = 256;
    std::int32_t sourceHeight = 256;
    ImagePattern pattern = ImagePattern::LinearGradient;
    Argb colorA = 0xFF000000u;
    Argb colorB = 0xFFFFFFFFu;
    std::uint32_t seed = 0;
};

struct StrokePoint
{
    double x = 0;
    double y = 0;
    float pressure = 0.5f;
};

struct Stroke
{
    std::vector<StrokePoint> points;
    double baseWidth = 3;
    Argb color = 0xFF000000u;
};

// Largeur rendue d'un point : base × (0,5 + pression) (hypothèse N2-2).
inline double strokeWidthAt(double baseWidth, float pressure) noexcept
{
    return baseWidth * (0.5 + static_cast<double>(pressure));
}

using PageObject = std::variant<Shape, ImageObject, Stroke>;

ObjectKind kindOf(const PageObject& object) noexcept;

// Objets dans l'ordre d'empilement : le dernier est au premier plan.
struct Page
{
    double width = 1024;
    double height = 768;
    std::vector<PageObject> objects;
};

// Empreinte FNV-1a 64 bits du contenu ; identique sur toutes les plateformes
// (types à taille fixe, octets dans un ordre fixé, réels par leur motif binaire IEEE 754).
std::uint64_t fingerprint(const Page& page) noexcept;

} // namespace actcraft::core
