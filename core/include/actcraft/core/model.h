#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
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
// Inverse de `t` ; rien si elle n'existe pas (déterminant nul ou non fini).
std::optional<Transform> inverted(const Transform& t) noexcept;
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

    bool operator==(const Fill&) const = default;
};

inline Fill solidFill(Argb color) noexcept
{
    Fill f;
    f.kind = FillKind::Solid;
    f.color = color;
    return f;
}

// Déplacement d'un objet (lot 8, F-578, D8-7). Fixed : bloqué (movable faux).
enum class Travel : std::uint8_t
{
    Free = 0,
    Vertical = 1,
    Horizontal = 2,
    AlongTrack = 3,
    Fixed = 4,
};

// Rotation d'un objet (lot 8, F-579). Fixed : bloquée (rotatable faux).
enum class Turning : std::uint8_t
{
    Free = 0,
    Clockwise = 1,
    CounterClockwise = 2,
    Fixed = 3,
};

// Repère d'un objet : pivot de rotation, point d'aimantation.
enum class Mark : std::uint8_t
{
    Center = 0,
    TopLeft = 1,
    Top = 2,
    TopRight = 3,
    Right = 4,
    BottomRight = 5,
    Bottom = 6,
    BottomLeft = 7,
    Left = 8,
    FirstTextLine = 9,
    Point = 10,
    OtherObject = 11,
};

struct Restrictions
{
    bool movable = true;
    bool resizable = true;
    bool rotatable = true;
    // Lot 8 (F-578, F-579, D8-7). Les booléens restent la source du « bloqué » : interactivity.h les tient
    // d'accord.
    Travel travel = Travel::Free;
    ObjectId track = 0; // objet trait ou ligne suivi (Travel::AlongTrack)
    Turning turning = Turning::Free;
    double turnStep = 0; // degrés ; 0 : continu
    Mark pivot = Mark::Center;
    PointF pivotPoint;        // Mark::Point, coordonnées de l'objet
    ObjectId pivotObject = 0; // Mark::OtherObject
    bool blocksOthers = false;
    bool magnet = false; // l'objet se pose sur `magnetMark` du dernier conteneur ou de `magnetPoint`
    Mark magnetMark = Mark::Center;
    PointF magnetPoint;
};

enum class Accepts : std::uint8_t
{
    Nothing = 0,
    Anything = 1,
    OneObject = 2,
    Keywords = 3,
};

enum class FitRule : std::uint8_t
{
    CentresMatch = 0,
    FullyInside = 1,
};

// Conteneur (F-577, F-580, D8-6).
struct ContainerRule
{
    Accepts accepts = Accepts::Nothing;
    ObjectId object = 0;               // Accepts::OneObject
    std::vector<std::string> keywords; // Accepts::Keywords
    FitRule fit = FitRule::CentresMatch;
    double tolerance = 10;   // pixels de page (centres)
    bool bounceBack = false; // retour si non contenu
    bool chime = false;      // son de confirmation
    std::string chimeSound;  // clé de ressource du son
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
    std::string resource;    // clé de ressource (fichier ouvert) ; vide sinon
    std::string unknownKeys; // clés de l'élément inconnues de cette version (comme PageObject::unknownKeys)
    // Lot 8 : action du catalogue d'ActCraft (vide : non prise en charge ; kind, sourceType et command, lus
    // du fichier, ne sont jamais réécrits), ses paramètres (JSON compact), déclenchée par un glisser-déposer.
    std::string catalogId;
    std::string parameters;
    bool dragDrop = false;
};

// Style de la légende d'un objet (F-576), écrit par nous. Valeurs par défaut : pas de clé dans le fichier,
// rien dans l'empreinte.
struct LabelStyle
{
    std::string fontFamily; // vide : police de l'interface
    double pointSize = 12;
    Argb color = 0xFF000000;
    PenStyle outline = PenStyle::None; // contour du cadre de la légende
    Argb outlineColor = 0xFF000000;
    Argb fill = 0x00000000;  // fond ; transparent par défaut
    std::string unknownKeys; // sous-clés inconnues gardées (JSON compact), réécrites avec le style
    bool operator==(const LabelStyle&) const = default;
};

// Propriétés communes à tout objet (décision 15).
struct ObjectCommon
{
    ObjectId id = 0;
    std::string name;
    Layer layer = Layer::Middle;
    bool visible = true;
    bool locked = false;
    bool dragCopy = false;         // F-587 : chaque glissement dépose une copie, l'original reste
    std::uint8_t translucency = 0; // 0 opaque … 255 invisible : opacité = 1 − t / 255
    Restrictions restrictions;
    Transform transform;               // coordonnées de l'objet → coordonnées de page
    std::string label;                 // légende (F-576) ; vide : aucune
    LabelStyle labelStyle;             // style de la légende (F-576, L6-T10)
    std::vector<std::string> keywords; // mots-clés (F-570, L6-T10) ; vide : aucun
    std::vector<Action> actions;
    ContainerRule container; // lot 8 (F-577) ; défaut : n'accepte rien
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
    Equation = 8,
    WebFrame = 9, // lot 8 (F-302)
    Unknown = 10, // type que cette version ne connaît pas (UnknownObject) ; toujours le dernier
};
inline constexpr std::size_t kObjectKindCount = 11;

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
    std::string preset; // préréglage de la palette (« rectangle »…) ; vide : forme lue ou générique
    std::vector<double> parameters; // points intelligents du préréglage (F-242)
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
    bool hasKeyColor = false;
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
    std::string unknownKeys; // clés de l'élément inconnues de cette version (comme PageObject::unknownKeys)
};

// Texte riche (décision 37) : HTML pour QTextDocument ; `runs`, s'il y en a, fait foi pour le dessin.
struct Text
{
    RectF bounds;
    std::string html;
    std::vector<TextSpan> runs;
    bool vertical = false; // texte de haut en bas (F-301) : colonne d'un caractère
    // Origine de la ligne de base de la première ligne, coordonnées de l'objet, mesurée à la validation sur
    // le document du peintre (H4-6, L4-C5). Absente (texte lu, colonne, texte du banc) : l'accrochage prend
    // la première ligne lue, sinon l'emprise.
    std::optional<PointF> firstBaseline;
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
    // Options du lot 8 (F-302).
    bool controlsHidden = false;
    bool aspectLocked = false;
    bool clearBackground = false;
    bool wrapText = false;
};

// Groupe par références (F-500) : les membres restent des objets de la page, à transformation absolue.
struct Group
{
    std::vector<ObjectId> members;
};

// Extrémité d'un connecteur (F-245).
enum class ConnectorCap : std::uint8_t
{
    None = 0,
    Round = 1,
    Arrow = 2,
};

struct Connector
{
    PointF firstEnd;
    PointF secondEnd;
    ObjectId objectA = 0;
    ObjectId objectB = 0;
    Outline pen;
    std::uint8_t style = 0; // rang dans le panneau des connecteurs (0 : ligne simple), D11
    ConnectorCap capA = ConnectorCap::None;
    ConnectorCap capB = ConnectorCap::None;
};

// Équation (F-301) : MathML gardé tel quel, jamais interprété au socle ; dessinée par un repère neutre sur
// `bounds` (coordonnées de l'objet) jusqu'à sa mise en page (lot 7).
struct Equation
{
    RectF bounds;
    std::string mathml;
    double pointSize = 24;
    Argb color = 0xFF000000u;
};

// Objet d'un type que cette version ne connaît pas (fichier d'une version plus récente, L2-C8) : gardé tel
// quel et réécrit à l'identique ; montré par un repère neutre ; jamais modifié par cette version. `json` :
// l'objet JSON lu, toutes ses clés (« type » compris), en texte UTF-8 compact, que formats/ seul interprète.
// `common` (PageObject) en est tiré pour la couche, l'ordre, l'identifiant et la transformation ; il n'est
// pas réécrit. `bounds` : boîte en coordonnées de l'objet, si le fichier en donne une lisible.
// Cadre web (F-302, S27) : code HTML saisi, ou adresse ; jamais chargé sans la politique de L8-T29.
struct WebFrame
{
    RectF bounds;
    std::string html;
    std::string address;
};

struct UnknownObject
{
    std::string type;
    std::string json;
    RectF bounds;
    bool hasBounds = false;
};

using ObjectVariant = std::variant<Shape, ImageObject, Stroke, Line, Text, Media, Group, Connector, Equation,
                                   WebFrame, UnknownObject>;

// Objet de page : le variant de son type (std::get_if, std::get, index() marchent tels quels) et ses
// propriétés communes. Ne pas appeler std::visit dessus : passer par body().
struct PageObject : ObjectVariant
{
    using ObjectVariant::ObjectVariant;
    using ObjectVariant::operator=;
    ObjectCommon common;
    // Clés de l'objet que cette version ne connaît pas (version plus récente, L2-C8) : objet JSON compact des
    // seuls membres inconnus, à toute profondeur d'objets, réécrits tels quels ; vide : aucune.
    std::string unknownKeys;
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
    std::string unknownKeys; // clés de l'élément inconnues de cette version (comme PageObject::unknownKeys)
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

// Réglages de présentation propres à la page (F-591 à F-593, F-640, F-284) : outils de page à l'ouverture,
// rideau, projecteur, origine XY, effet de transition. Aucun ne change un pixel du contenu fixe (cache).
enum class PageToolsMode : std::uint8_t
{
    Keep = 0,
    Curtain = 1,
    Spotlight = 2,
    Off = 3,
};
enum class CurtainMode : std::uint8_t
{
    Off = 0,
    Top = 1,
    Bottom = 2,
    Left = 3,
    Right = 4,
    Full = 5,
};
enum class SpotlightShape : std::uint8_t
{
    Off = 0,
    Circle = 1,
    Square = 2,
    SolidCircle = 3,
    SolidSquare = 4,
};
struct PagePresentation
{
    PageToolsMode tools = PageToolsMode::Keep;
    CurtainMode curtain = CurtainMode::Off;
    double curtainDistance = 0; // pixels de page depuis le bord
    SpotlightShape spotlight = SpotlightShape::Off;
    RectF spotlightFrame; // vide : cadre par défaut
    bool hasOrigin = false;
    PointF origin;          // origine XY mémorisée (F-284)
    std::string transition; // identifiant d'effet propre à la page ; vide : effet global
    // Sous-clés inconnues de "presentation", JSON compact, réécrites telles quelles (L2-C8).
    std::string unknownKeys;
};
// Vrai : rien à écrire, rien dans l'empreinte. Faux dès qu'un champ diffère de son défaut ou qu'une sous-clé
// inconnue est gardée.
bool isDefault(const PagePresentation& presentation) noexcept;

// Zones de page (F-603, D11) : la page compte des cadres, `horizontal` en largeur (0 à 40) et `vertical` en
// hauteur (0 à 53), chacun d'une taille en pixels. Bornes : clampZones (pagezones.h). Tout à 0 : aucune zone,
// rien dans l'empreinte ni dans le fichier.
struct PageZones
{
    std::int32_t horizontal = 0;
    std::int32_t vertical = 0;
    double frameWidth = 0;
    double frameHeight = 0; // taille d'un cadre en pixels (D11) ; 0 : non donnée
};
// Vrai : les quatre champs sont nuls, rien à écrire, rien dans l'empreinte.
bool isDefault(const PageZones& zones) noexcept;

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
    std::string description;           // description de la page (F-570, L6-T10) ; vide : aucune
    std::vector<std::string> keywords; // mots-clés de la page (F-570, L6-T10) ; vide : aucun
    PagePresentation presentation;
    // Clés de la page que cette version ne connaît pas (comme PageObject::unknownKeys) ; vide : aucune.
    std::string unknownKeys;
    double baseHeight = 0; // hauteur avant la première extension (F-301) ; 0 : jamais allongée
    PageZones zones;       // cadres de la page (F-603, D11, L4-T27) ; l'allongement ne les modifie pas
};

// Empreinte FNV-1a 64 bits du contenu ; identique sur toutes les plateformes (types à taille fixe, octets
// dans un ordre fixé, réels par leur motif binaire IEEE 754, textes par leurs octets UTF-8 précédés de leur
// longueur). Les clés inconnues (unknownKeys, L2-C8) de la page et de ses objets n'y entrent qu'en fin de
// flux et s'il y en a, celles d'une action, d'une ligne de texte ou d'un niveau de grille avec l'élément :
// une page qui n'en a pas garde son empreinte d'avant L2-C8. Les réglages de présentation n'y entrent que
// s'ils ne sont pas ceux par défaut (isDefault), en fin de flux avant les clés inconnues : une page sans eux
// garde son empreinte d'avant le socle des lots 3 et 7. Les données des lots 4 et 6 (mots-clés, description,
// style de légende) n'y entrent qu'en toute fin, et seulement hors défaut : une page qui n'en a pas garde son
// empreinte d'avant, une page qui en a prolonge le flux de la même page sans elles.
std::uint64_t fingerprint(const Page& page) noexcept;

// Diffusion protégée (F-680, S18) : courtoisie sur un format ouvert (N1-A4). Portée par core::Document.
struct BroadcastRules
{
    bool noCopy = false;
    bool noSave = false;
    bool noPrint = false;
    bool noCamera = false;
    std::string author; // informations de l'auteur, jamais affichées aux utilisateurs
    bool isDefault() const noexcept { return !noCopy && !noSave && !noPrint && !noCamera && author.empty(); }
};

} // namespace actcraft::core
