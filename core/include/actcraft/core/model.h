#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

// Modèle d'une page (DA2, décision 15 du cadrage livraison-complete). Sans QtGui : couleurs en entiers,
// géométrie en doubles, textes en UTF-8. Les trois types du prototype (Shape, ImageObject, Stroke) gardent
// leurs champs et leur rang dans le variant : générateur du banc et moteurs de rendu inchangés.

namespace actcraft::core {

// Couleur ARGB 32 bits (0xAARRGGBB), même codage que QRgb.
using Argb = std::uint32_t;
// Identifiant stable d'un objet dans son document (décision 15) ; 0 = aucun.
using ObjectId = std::uint64_t;

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

// Transformation affine 2D, convention de QTransform : x' = m11·x + m21·y + dx ; y' = m12·x + m22·y + dy.
struct Transform
{
    double m11 = 1;
    double m12 = 0;
    double m21 = 0;
    double m22 = 1;
    double dx = 0;
    double dy = 0;

    PointF map(PointF p) const noexcept { return {m11 * p.x + m21 * p.y + dx, m12 * p.x + m22 * p.y + dy}; }
    bool isIdentity() const noexcept
    {
        return m11 == 1 && m12 == 0 && m21 == 0 && m22 == 1 && dx == 0 && dy == 0;
    }
};

// `first` appliquée d'abord, puis `second` (QTransform : first * second).
Transform compose(const Transform& first, const Transform& second) noexcept;
Transform translation(double dx, double dy) noexcept;
Transform scaling(double sx, double sy) noexcept;
// Degrés, sens horaire à l'écran (y vers le bas), comme QTransform::rotate ; angles droits exacts.
Transform rotation(double degrees) noexcept;

// Quatre couches (F-510), de bas en haut. Page::objects est rangé par couche (isLayerOrdered, edits.h).
enum class Layer : std::uint8_t
{
    Background = 0,
    Bottom = 1,
    Middle = 2,
    Top = 3,
};

// Valeurs de Qt::PenStyle.
enum class PenStyle : std::uint8_t
{
    None = 0,
    Solid = 1,
    Dash = 2,
    Dot = 3,
    DashDot = 4,
    DashDotDot = 5,
};

enum class PenCap : std::uint8_t
{
    Flat = 0,
    Square = 1,
    Round = 2,
};

enum class PenJoin : std::uint8_t
{
    Miter = 0,
    Bevel = 1,
    Round = 2,
};

struct Outline
{
    PenStyle style = PenStyle::Solid;
    Argb color = 0xFF000000u;
    double width = 1;
    PenCap cap = PenCap::Round;
    PenJoin join = PenJoin::Round;
};

enum class FillKind : std::uint8_t
{
    None = 0,
    Solid = 1,
    Pattern = 2,
    Gradient = 3,
};

enum class GradientKind : std::uint8_t
{
    Vertical = 1,
    Horizontal = 2,
    DiagonalDown = 3, // coin haut gauche → bas droit
    DiagonalUp = 4,   // coin haut droit → bas gauche
    Radial = 5,
};

struct Fill
{
    FillKind kind = FillKind::None;
    Argb color = 0xFFFFFFFFu;
    Argb color2 = 0xFFFFFFFFu; // dégradé
    GradientKind gradient = GradientKind::Vertical;
    std::uint8_t pattern = 0;      // Qt::BrushStyle 2 à 14 (FillKind::Pattern)
    bool opaqueBackground = false; // F-574 : fond des motifs
    Argb backgroundColor = 0xFF000000u;
};

inline Fill solidFill(Argb color) noexcept
{
    Fill f;
    f.kind = FillKind::Solid;
    f.color = color;
    return f;
}

struct Restrictions
{
    bool movable = true;
    bool resizable = true;
    bool rotatable = true;
};

// Actions : données seulement au lot 2 (F-583) ; leur comportement arrive au lot 8.
enum class ActionKind : std::uint8_t
{
    Other = 0,
    Navigate = 1,
    OpenExternal = 2,
    OnObject = 3,
};

struct Action
{
    ActionKind kind = ActionKind::Other;
    std::uint16_t sourceType = 0; // numéro de type .flipchart d'origine ; 0 : action créée par ActCraft
    std::int32_t command = 0;
    ObjectId target = 0;
    std::string resource; // clé de ressource (fichier ouvert) ; vide sinon
};

// Propriétés communes à tout objet (décision 15).
struct ObjectCommon
{
    ObjectId id = 0;
    std::string name;
    Layer layer = Layer::Middle;
    bool visible = true;
    bool locked = false;
    std::uint8_t translucency = 0; // 0 opaque … 255 invisible : opacité = 1 − t / 255
    Restrictions restrictions;
    Transform transform; // coordonnées de l'objet → coordonnées de page
    std::string label;   // légende (F-576) ; vide : aucune
    std::vector<Action> actions;
};

// L'ordre suit celui d'ObjectVariant : kindOf() en dépend.
enum class ObjectKind : std::uint8_t
{
    Shape = 0,
    Image = 1,
    Stroke = 2,
    Line = 3,
    Text = 4,
    Media = 5,
    Group = 6,
    Connector = 7,
};
inline constexpr std::size_t kObjectKindCount = 8;

enum class ShapeKind : std::uint8_t
{
    Rectangle = 0,
    Ellipse = 1,
    Polygon = 2,
    Path = 3, // formes lues : sous-chemins, plume et remplissage complets
};

struct Shape
{
    ShapeKind kind = ShapeKind::Rectangle;
    RectF bounds;
    std::vector<PointF> polygon; // sommets (Polygon seulement)
    Argb fill = 0xFFFFFFFFu;
    Argb outline = 0xFF000000u;
    double outlineWidth = 1; // 0 : pas de contour
    double opacity = 1;
    // ShapeKind::Path : sous-chemins en coordonnées de l'objet, dessinés avec `pen` et `brush` (les quatre
    // champs de couleur ci-dessus ne servent pas). `bounds` : rectangle englobant lu (épaisseur de plume
    // comprise).
    std::vector<std::vector<PointF>> paths;
    bool open = false;
    bool windingFill = false;
    Outline pen;
    Fill brush;
};

enum class ImagePattern : std::uint8_t
{
    LinearGradient = 0,
    Checker = 1,
    Rings = 2,
    SolidCenter = 3, // moitié centrale unie (colorA), bord colorB : objet témoin
};

// Image : procédurale (banc, `resource` vide) ou ressource du document (`resource` = clé SHA-256).
struct ImageObject
{
    RectF bounds;
    std::int32_t sourceWidth = 256;
    std::int32_t sourceHeight = 256;
    ImagePattern pattern = ImagePattern::LinearGradient;
    Argb colorA = 0xFF000000u;
    Argb colorB = 0xFFFFFFFFu;
    std::uint32_t seed = 0;
    std::string resource;
    bool hasTransparentColor = false;
    Argb keyColor = 0;
};

struct StrokePoint
{
    double x = 0;
    double y = 0;
    float pressure = 0.5f;
};

enum class StrokeKind : std::uint8_t
{
    Pen = 0,
    Highlighter = 1,
    Revealer = 2, // encre révélatrice (N1-12)
};

struct Stroke
{
    std::vector<StrokePoint> points;
    double baseWidth = 3;
    Argb color = 0xFF000000u;
    StrokeKind kind = StrokeKind::Pen;
    PenCap cap = PenCap::Square; // bouts carrés : moteur du prototype ; les annotations lues sont rondes
};

// Largeur rendue d'un point : base × (0,5 + pression) (hypothèse N2-2).
inline double strokeWidthAt(double baseWidth, float pressure) noexcept
{
    return baseWidth * (0.5 + static_cast<double>(pressure));
}

struct Line
{
    std::vector<PointF> points;
    Outline pen;
};

// Ligne d'un texte mise en page à l'avance (.flipchart) ; vide pour un texte créé par ActCraft.
struct TextSpan
{
    std::string text;
    std::string fontFamily;
    double pointSize = 12;
    std::int32_t weight = 400; // échelle de Qt 6 (100 à 900)
    bool italic = false;
    bool underline = false;
    bool strikeOut = false;
    Argb color = 0xFF000000u;
    PointF baseline; // origine de la ligne de base, coordonnées de l'objet
    RectF bounds;
};

// Texte riche (décision 37) : HTML pour QTextDocument ; `runs`, s'il y en a, fait foi pour le dessin.
struct Text
{
    RectF bounds;
    std::string html;
    std::vector<TextSpan> runs;
};

// Son ou vidéo (N2-9 : ouvert par l'application du système) ; dessiné par son image d'espace réservé.
struct Media
{
    RectF bounds;
    std::string placeholder; // clé de ressource image ; vide : cadre par défaut
    std::string media;       // clé de ressource du son ou de la vidéo
    std::string fileName;
    bool playOnOpen = false;
    bool loop = false;
};

// Groupe par références (F-500) : les membres restent des objets de la page, à transformation absolue.
struct Group
{
    std::vector<ObjectId> members;
};

struct Connector
{
    PointF firstEnd;
    PointF secondEnd;
    ObjectId objectA = 0;
    ObjectId objectB = 0;
    Outline pen;
};

using ObjectVariant = std::variant<Shape, ImageObject, Stroke, Line, Text, Media, Group, Connector>;

// Objet de page : le variant de son type (std::get_if, std::get, index() marchent tels quels) et ses
// propriétés communes. Ne pas appeler std::visit dessus : passer par body().
struct PageObject : ObjectVariant
{
    using ObjectVariant::ObjectVariant;
    using ObjectVariant::operator=;
    ObjectCommon common;
};

inline const ObjectVariant& body(const PageObject& object) noexcept
{
    return object;
}
inline ObjectVariant& body(PageObject& object) noexcept
{
    return object;
}

ObjectKind kindOf(const PageObject& object) noexcept;

// Grille à niveaux (F-620, F-621) ; dessin d'après REA - Activinspire/app/actcraft/canvas/grid.py.
enum class GridPattern : std::uint8_t
{
    Normal = 0,
    Diagonal45 = 1,
    Angle = 2,
};

enum class GridLine : std::uint8_t
{
    Points = 0,
    Cross = 1,
    Solid = 2,
    Dotted = 3,
    Dashed = 4,
    BigDots = 5,
};

struct GridLevel
{
    bool visible = true;
    bool snap = true;
    PointF start;
    double spacingX = 32;
    double spacingY = 32;
    Argb color = 0xFF000000u;
    GridPattern pattern = GridPattern::Normal;
    double angle = 30;
    GridLine line = GridLine::Solid;
    bool linesAlongX = true;
    bool linesAlongY = true;
};

struct Grid
{
    bool visible = false;
    bool snapEnabled = false;
    bool drawnAbove = false;
    double previewZoom = 1;
    double baseZoom = 1;
    double zoomIncrement = 0.5;
    std::vector<GridLevel> levels;
};

// Fond de page (F-610) : couleur ou dégradé, image éventuelle par-dessus (clé de ressource).
struct Background
{
    Fill fill = solidFill(0xFFFFFFFFu);
    std::string image;
};

// Objets dans l'ordre de dessin, rangés par couche : le dernier est au premier plan.
struct Page
{
    double width = 1024;
    double height = 768;
    std::vector<PageObject> objects;
    Background background;
    Grid grid;
    std::string name;
    std::string notes;
};

// Empreinte FNV-1a 64 bits du contenu ; identique sur toutes les plateformes (types à taille fixe, octets
// dans un ordre fixé, réels par leur motif binaire IEEE 754, textes par leurs octets UTF-8 précédés de leur
// longueur).
std::uint64_t fingerprint(const Page& page) noexcept;

} // namespace actcraft::core
