#include <actcraft/formats/nativejson.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

// Parité écrivain/lecteur (DA3) : chaque structure du modèle est écrite ET relue par une liaison structurée
// qui nomme tous ses champs, comme le hachage de core/src/model.cpp (hashType, fingerprint) : un champ ajouté
// au modèle casse la compilation ici, dans l'écrivain comme dans le lecteur, au lieu de disparaître du
// fichier. Un type ajouté au variant tombe sur le modèle writeBody/readBody et son static_assert.
//
// Ce qu'on ne reprend PAS du hachage : ses valeurs numériques d'énumérations (Fnv1a::tag). Une empreinte ne
// quitte pas la mémoire ; un fichier, si : chaque énumération y est écrite par son nom, un par un, dans les
// tables ci-dessous. Un nombre rendrait le fichier illisible et changerait de sens au premier
// réordonnancement.

namespace actcraft::formats {

using namespace Qt::StringLiterals;
using namespace actcraft::core;

namespace {

// --- Noms des énumérations -----------------------------------------------------------------------------

// Un switch sans « default » par énumération, seul endroit qui nomme ses valeurs : un énumérateur ajouté au
// modèle sans son nom ne compile pas (-Wswitch sous Clang et GCC ; C4062, mis en erreur ci-dessous, sous
// MSVC). Une valeur hors énumération (conversion fautive) a un nom vide, que la lecture signale. Les tables
// de lecture en sont déduites à la compilation (namesOf).
#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(error : 4062)
#endif

constexpr std::string_view enumName(Layer value) noexcept
{
    switch (value) {
    case Layer::Background:
        return "background";
    case Layer::Bottom:
        return "bottom";
    case Layer::Middle:
        return "middle";
    case Layer::Top:
        return "top";
    }
    return {};
}

constexpr std::string_view enumName(PenStyle value) noexcept
{
    switch (value) {
    case PenStyle::None:
        return "none";
    case PenStyle::Solid:
        return "solid";
    case PenStyle::Dash:
        return "dash";
    case PenStyle::Dot:
        return "dot";
    case PenStyle::DashDot:
        return "dashDot";
    case PenStyle::DashDotDot:
        return "dashDotDot";
    }
    return {};
}

constexpr std::string_view enumName(PenCap value) noexcept
{
    switch (value) {
    case PenCap::Flat:
        return "flat";
    case PenCap::Square:
        return "square";
    case PenCap::Round:
        return "round";
    }
    return {};
}

constexpr std::string_view enumName(PenJoin value) noexcept
{
    switch (value) {
    case PenJoin::Miter:
        return "miter";
    case PenJoin::Bevel:
        return "bevel";
    case PenJoin::Round:
        return "round";
    }
    return {};
}

constexpr std::string_view enumName(FillKind value) noexcept
{
    switch (value) {
    case FillKind::None:
        return "none";
    case FillKind::Solid:
        return "solid";
    case FillKind::Pattern:
        return "pattern";
    case FillKind::Gradient:
        return "gradient";
    }
    return {};
}

constexpr std::string_view enumName(GradientKind value) noexcept
{
    switch (value) {
    case GradientKind::Vertical:
        return "vertical";
    case GradientKind::Horizontal:
        return "horizontal";
    case GradientKind::DiagonalDown:
        return "diagonalDown";
    case GradientKind::DiagonalUp:
        return "diagonalUp";
    case GradientKind::Radial:
        return "radial";
    }
    return {};
}

constexpr std::string_view enumName(ActionKind value) noexcept
{
    switch (value) {
    case ActionKind::Other:
        return "other";
    case ActionKind::Navigate:
        return "navigate";
    case ActionKind::OpenExternal:
        return "openExternal";
    case ActionKind::OnObject:
        return "onObject";
    }
    return {};
}

constexpr std::string_view enumName(ObjectKind value) noexcept
{
    switch (value) {
    case ObjectKind::Shape:
        return "shape";
    case ObjectKind::Image:
        return "image";
    case ObjectKind::Stroke:
        return "stroke";
    case ObjectKind::Line:
        return "line";
    case ObjectKind::Text:
        return "text";
    case ObjectKind::Media:
        return "media";
    case ObjectKind::Group:
        return "group";
    case ObjectKind::Connector:
        return "connector";
    case ObjectKind::Equation:
        return "equation";
    case ObjectKind::Unknown: // le nom est celui du fichier (UnknownObject::type), jamais un nom d'ici
        return {};
    }
    return {};
}

constexpr std::string_view enumName(ShapeKind value) noexcept
{
    switch (value) {
    case ShapeKind::Rectangle:
        return "rectangle";
    case ShapeKind::Ellipse:
        return "ellipse";
    case ShapeKind::Polygon:
        return "polygon";
    case ShapeKind::Path:
        return "path";
    }
    return {};
}

constexpr std::string_view enumName(ImagePattern value) noexcept
{
    switch (value) {
    case ImagePattern::LinearGradient:
        return "linearGradient";
    case ImagePattern::Checker:
        return "checker";
    case ImagePattern::Rings:
        return "rings";
    case ImagePattern::SolidCenter:
        return "solidCenter";
    }
    return {};
}

constexpr std::string_view enumName(StrokeKind value) noexcept
{
    switch (value) {
    case StrokeKind::Pen:
        return "pen";
    case StrokeKind::Highlighter:
        return "highlighter";
    case StrokeKind::Revealer:
        return "revealer";
    }
    return {};
}

constexpr std::string_view enumName(GridPattern value) noexcept
{
    switch (value) {
    case GridPattern::Normal:
        return "normal";
    case GridPattern::Diagonal45:
        return "diagonal45";
    case GridPattern::Angle:
        return "angle";
    }
    return {};
}

constexpr std::string_view enumName(GridLine value) noexcept
{
    switch (value) {
    case GridLine::Points:
        return "points";
    case GridLine::Cross:
        return "cross";
    case GridLine::Solid:
        return "solid";
    case GridLine::Dotted:
        return "dotted";
    case GridLine::Dashed:
        return "dashed";
    case GridLine::BigDots:
        return "bigDots";
    }
    return {};
}

constexpr std::string_view enumName(PageToolsMode value) noexcept
{
    switch (value) {
    case PageToolsMode::Keep:
        return "keep";
    case PageToolsMode::Curtain:
        return "curtain";
    case PageToolsMode::Spotlight:
        return "spotlight";
    case PageToolsMode::Off:
        return "off";
    }
    return {};
}

constexpr std::string_view enumName(CurtainMode value) noexcept
{
    switch (value) {
    case CurtainMode::Off:
        return "off";
    case CurtainMode::Top:
        return "top";
    case CurtainMode::Bottom:
        return "bottom";
    case CurtainMode::Left:
        return "left";
    case CurtainMode::Right:
        return "right";
    case CurtainMode::Full:
        return "full";
    }
    return {};
}

constexpr std::string_view enumName(SpotlightShape value) noexcept
{
    switch (value) {
    case SpotlightShape::Off:
        return "off";
    case SpotlightShape::Circle:
        return "circle";
    case SpotlightShape::Square:
        return "square";
    case SpotlightShape::SolidCircle:
        return "solid-circle";
    case SpotlightShape::SolidSquare:
        return "solid-square";
    }
    return {};
}

template <typename E> struct Named
{
    E value;
    std::string_view name;
};

// Valeurs nommées de E, dans l'ordre des valeurs : E tient sur un octet, les 256 valeurs sont essayées à la
// compilation.
template <typename E> constexpr std::size_t namedCount()
{
    static_assert(std::is_same_v<std::underlying_type_t<E>, std::uint8_t>,
                  "énumération sur un octet attendue (namesOf essaie les 256 valeurs)");
    std::size_t count = 0;
    for (unsigned u = 0; u <= 0xFFu; ++u) {
        if (!enumName(static_cast<E>(u)).empty()) {
            ++count;
        }
    }
    return count;
}

template <typename E> constexpr std::array<Named<E>, namedCount<E>()> namesOf()
{
    std::array<Named<E>, namedCount<E>()> table{};
    std::size_t k = 0;
    for (unsigned u = 0; u <= 0xFFu; ++u) {
        const E value = static_cast<E>(u);
        if (!enumName(value).empty()) {
            table[k].value = value;
            table[k].name = enumName(value);
            ++k;
        }
    }
    return table;
}

constexpr auto kLayers = namesOf<Layer>();
constexpr auto kPenStyles = namesOf<PenStyle>();
constexpr auto kPenCaps = namesOf<PenCap>();
constexpr auto kPenJoins = namesOf<PenJoin>();
constexpr auto kFillKinds = namesOf<FillKind>();
constexpr auto kGradients = namesOf<GradientKind>();
constexpr auto kActionKinds = namesOf<ActionKind>();
constexpr auto kObjectKinds = namesOf<ObjectKind>();
constexpr auto kShapeKinds = namesOf<ShapeKind>();
constexpr auto kPatterns = namesOf<ImagePattern>();
constexpr auto kStrokeKinds = namesOf<StrokeKind>();
constexpr auto kGridPatterns = namesOf<GridPattern>();
constexpr auto kGridLines = namesOf<GridLine>();
constexpr auto kToolsModes = namesOf<PageToolsMode>();
constexpr auto kCurtainModes = namesOf<CurtainMode>();
constexpr auto kSpotlightShapes = namesOf<SpotlightShape>();

// Un nom ou une valeur en double : la relecture rendrait une autre valeur que celle écrite.
template <typename E, std::size_t N> constexpr bool distinct(const std::array<Named<E>, N>& table)
{
    for (std::size_t i = 0; i < N; ++i) {
        for (std::size_t k = i + 1; k < N; ++k) {
            if (table[i].value == table[k].value || table[i].name == table[k].name) {
                return false;
            }
        }
    }
    return true;
}
static_assert(distinct(kLayers) && distinct(kPenStyles) && distinct(kPenCaps) && distinct(kPenJoins) &&
                  distinct(kFillKinds) && distinct(kGradients) && distinct(kActionKinds) &&
                  distinct(kObjectKinds) && distinct(kShapeKinds) && distinct(kPatterns) &&
                  distinct(kStrokeKinds) && distinct(kGridPatterns) && distinct(kGridLines) &&
                  distinct(kToolsModes) && distinct(kCurtainModes) && distinct(kSpotlightShapes),
              "document.json : nom ou valeur d'énumération en double");

constexpr bool followsVariantOrder()
{
    for (std::size_t i = 0; i < kObjectKinds.size(); ++i) {
        if (static_cast<std::size_t>(kObjectKinds[i].value) != i) {
            return false;
        }
    }
    return kObjectKinds.size() + 1 == kObjectKindCount &&
           static_cast<std::size_t>(ObjectKind::Unknown) + 1 == kObjectKindCount &&
           kObjectKindCount == std::variant_size_v<ObjectVariant>;
}
static_assert(
    followsVariantOrder(),
    "document.json : un nom par type d'objet connu, dans l'ordre du variant ; Unknown, sans nom, en dernier");

QString latin1(std::string_view text)
{
    return QString::fromLatin1(text.data(), static_cast<qsizetype>(text.size()));
}

template <typename E> QString nameText(E value)
{
    return latin1(enumName(value));
}

template <typename E, std::size_t N>
std::optional<E> valueOf(const std::array<Named<E>, N>& table, const QString& text)
{
    for (const Named<E>& entry : table) {
        if (text == QLatin1StringView(entry.name.data(), static_cast<qsizetype>(entry.name.size()))) {
            return entry.value;
        }
    }
    return std::nullopt;
}

// --- Écriture ------------------------------------------------------------------------------------------

QString str(const std::string& s)
{
    return QString::fromStdString(s);
}

// « #aarrggbb », hexadécimal minuscule.
QString colorText(Argb c)
{
    return u"#%1"_s.arg(c, 8, 16, QLatin1Char('0'));
}

// Un réel JSON porte exactement les entiers jusqu'à 2^53 : au-delà, la lecture refuse l'identifiant.
double idValue(ObjectId id)
{
    return static_cast<double>(id);
}

QJsonArray pointJson(const PointF& point)
{
    const auto& [x, y] = point;
    return QJsonArray{x, y};
}

QJsonArray rectJson(const RectF& rect)
{
    const auto& [x, y, width, height] = rect;
    return QJsonArray{x, y, width, height};
}

// Points à plat : x, y, x, y…
QJsonArray pairsJson(const std::vector<PointF>& points)
{
    QJsonArray a;
    for (const PointF& point : points) {
        const auto& [x, y] = point;
        a.append(x);
        a.append(y);
    }
    return a;
}

QJsonArray transformJson(const Transform& transform)
{
    const auto& [m11, m12, m21, m22, dx, dy] = transform;
    return QJsonArray{m11, m12, m21, m22, dx, dy};
}

QJsonObject outlineJson(const Outline& pen)
{
    const auto& [style, color, width, cap, join] = pen;
    return QJsonObject{{u"style"_s, nameText(style)},
                       {u"color"_s, colorText(color)},
                       {u"width"_s, width},
                       {u"cap"_s, nameText(cap)},
                       {u"join"_s, nameText(join)}};
}

QJsonObject fillJson(const Fill& brush)
{
    const auto& [kind, color, color2, gradient, pattern, opaqueBackground, backgroundColor] = brush;
    return QJsonObject{{u"kind"_s, nameText(kind)},
                       {u"color"_s, colorText(color)},
                       {u"color2"_s, colorText(color2)},
                       {u"gradient"_s, nameText(gradient)},
                       {u"pattern"_s, static_cast<int>(pattern)}, // Qt::BrushStyle : un nombre dans le modèle
                       {u"opaqueBackground"_s, opaqueBackground},
                       {u"backgroundColor"_s, colorText(backgroundColor)}};
}

// Objet JSON d'un texte gardé (UnknownObject::json, unknownKeys) ; vide s'il est vide ou n'en est pas un.
QJsonObject keptObject(const std::string& text)
{
    if (text.empty()) {
        return {};
    }
    return QJsonDocument::fromJson(QByteArray::fromStdString(text)).object();
}

// Ajoute à `written` les membres gardés `kept` (clés inconnues, L2-C8), sans jamais remplacer une clé écrite
// d'après le modèle : un membre d'un sous-objet connu (« pen », « background »…) reprend sa place dedans.
void mergeKept(QJsonObject& written, const QJsonObject& kept)
{
    for (auto it = kept.begin(); it != kept.end(); ++it) {
        const QJsonValue current = written.value(it.key());
        if (current.isUndefined()) {
            written.insert(it.key(), it.value());
        } else if (current.isObject() && it.value().isObject()) {
            QJsonObject inner = current.toObject();
            mergeKept(inner, it.value().toObject());
            written.insert(it.key(), inner);
        }
    }
}

QJsonObject withKept(QJsonObject written, const std::string& unknownKeys)
{
    mergeKept(written, keptObject(unknownKeys));
    return written;
}

QJsonObject actionJson(const Action& action)
{
    const auto& [kind, sourceType, command, target, resource, unknownKeys] = action;
    return withKept(QJsonObject{{u"kind"_s, nameText(kind)},
                                {u"sourceType"_s, static_cast<int>(sourceType)},
                                {u"command"_s, static_cast<int>(command)},
                                {u"target"_s, idValue(target)},
                                {u"resource"_s, str(resource)}},
                    unknownKeys);
}

void writeCommon(QJsonObject& j, const ObjectCommon& common)
{
    const auto& [id, name, layer, visible, locked, translucency, restrictions, transform, label, actions] =
        common;
    j.insert(u"id"_s, idValue(id));
    j.insert(u"name"_s, str(name));
    j.insert(u"layer"_s, nameText(layer));
    j.insert(u"visible"_s, visible);
    j.insert(u"locked"_s, locked);
    j.insert(u"translucency"_s, static_cast<int>(translucency));
    const auto& [movable, resizable, rotatable] = restrictions;
    j.insert(u"restrictions"_s,
             QJsonObject{{u"move"_s, movable}, {u"size"_s, resizable}, {u"rotate"_s, rotatable}});
    j.insert(u"transform"_s, transformJson(transform));
    j.insert(u"label"_s, str(label));
    QJsonArray list;
    for (const Action& action : actions) {
        list.append(actionJson(action));
    }
    j.insert(u"actions"_s, list);
}

// Un écrivain par type d'objet ; objectJson() appelle celui du type tenu.
template <typename T> void writeBody(QJsonObject&, const T&)
{
    static_assert(sizeof(T) == 0, "type d'objet sans écriture : écrire son writeBody() et son readBody() "
                                  "(formats/src/nativejson.cpp)");
}

void writeBody(QJsonObject& j, const Shape& shape)
{
    const auto& [kind, bounds, polygon, fill, outline, outlineWidth, opacity, paths, open, windingFill, pen,
                 brush, preset, parameters] = shape; // préréglage et paramètres : L4-T6
    j.insert(u"shape"_s, nameText(kind));
    j.insert(u"bounds"_s, rectJson(bounds));
    j.insert(u"polygon"_s, pairsJson(polygon));
    j.insert(u"fill"_s, colorText(fill));
    j.insert(u"outline"_s, colorText(outline));
    j.insert(u"outlineWidth"_s, outlineWidth);
    j.insert(u"opacity"_s, opacity);
    QJsonArray subPaths;
    for (const std::vector<PointF>& subPath : paths) {
        subPaths.append(pairsJson(subPath));
    }
    j.insert(u"paths"_s, subPaths);
    j.insert(u"open"_s, open);
    j.insert(u"windingFill"_s, windingFill);
    j.insert(u"pen"_s, outlineJson(pen));
    j.insert(u"brush"_s, fillJson(brush));
}

void writeBody(QJsonObject& j, const ImageObject& image)
{
    const auto& [bounds, sourceWidth, sourceHeight, pattern, colorA, colorB, seed, resource, hasKeyColor,
                 keyColor] = image;
    j.insert(u"bounds"_s, rectJson(bounds));
    j.insert(u"sourceWidth"_s, static_cast<int>(sourceWidth));
    j.insert(u"sourceHeight"_s, static_cast<int>(sourceHeight));
    j.insert(u"pattern"_s, nameText(pattern));
    j.insert(u"colorA"_s, colorText(colorA));
    j.insert(u"colorB"_s, colorText(colorB));
    j.insert(u"seed"_s, static_cast<double>(seed));
    j.insert(u"resource"_s, str(resource));
    // Deux clés, pas une couleur « null » sans transparence : le modèle garde les deux champs et l'empreinte
    // les hache tous les deux (unusedFieldsStillTravel).
    j.insert(u"hasKeyColor"_s, hasKeyColor);
    j.insert(u"keyColor"_s, colorText(keyColor));
}

void writeBody(QJsonObject& j, const Stroke& stroke)
{
    const auto& [points, baseWidth, color, kind, cap] = stroke;
    QJsonArray flat; // x, y, pression…
    for (const StrokePoint& point : points) {
        const auto& [x, y, pressure] = point;
        flat.append(x);
        flat.append(y);
        flat.append(static_cast<double>(pressure));
    }
    j.insert(u"points"_s, flat);
    j.insert(u"baseWidth"_s, baseWidth);
    j.insert(u"color"_s, colorText(color));
    j.insert(u"kind"_s, nameText(kind));
    j.insert(u"cap"_s, nameText(cap));
}

void writeBody(QJsonObject& j, const Line& line)
{
    const auto& [points, pen] = line;
    j.insert(u"points"_s, pairsJson(points));
    j.insert(u"pen"_s, outlineJson(pen));
}

QJsonObject runJson(const TextSpan& run)
{
    const auto& [text, fontFamily, pointSize, weight, italic, underline, strikeOut, color, baseline, bounds,
                 unknownKeys] = run;
    return withKept(QJsonObject{{u"text"_s, str(text)},
                                {u"fontFamily"_s, str(fontFamily)},
                                {u"pointSize"_s, pointSize},
                                {u"weight"_s, static_cast<int>(weight)},
                                {u"italic"_s, italic},
                                {u"underline"_s, underline},
                                {u"strikeOut"_s, strikeOut},
                                {u"color"_s, colorText(color)},
                                {u"baseline"_s, pointJson(baseline)},
                                {u"bounds"_s, rectJson(bounds)}},
                    unknownKeys);
}

void writeBody(QJsonObject& j, const Text& textObject)
{
    const auto& [bounds, html, runs, vertical] = textObject; // vertical : L4-T6
    j.insert(u"bounds"_s, rectJson(bounds));
    j.insert(u"html"_s, str(html));
    QJsonArray list;
    for (const TextSpan& run : runs) {
        list.append(runJson(run));
    }
    j.insert(u"runs"_s, list);
}

void writeBody(QJsonObject& j, const Media& mediaObject)
{
    const auto& [bounds, placeholder, media, fileName, playOnOpen, loop] = mediaObject;
    j.insert(u"bounds"_s, rectJson(bounds));
    j.insert(u"placeholder"_s, str(placeholder));
    j.insert(u"media"_s, str(media));
    j.insert(u"fileName"_s, str(fileName));
    j.insert(u"playOnOpen"_s, playOnOpen);
    j.insert(u"loop"_s, loop);
}

void writeBody(QJsonObject& j, const Group& group)
{
    const auto& [members] = group;
    QJsonArray list;
    for (const ObjectId id : members) {
        list.append(idValue(id));
    }
    j.insert(u"members"_s, list);
}

void writeBody(QJsonObject& j, const Connector& connector)
{
    const auto& [firstEnd, secondEnd, objectA, objectB, pen] = connector;
    j.insert(u"firstEnd"_s, pointJson(firstEnd));
    j.insert(u"secondEnd"_s, pointJson(secondEnd));
    j.insert(u"objectA"_s, idValue(objectA));
    j.insert(u"objectB"_s, idValue(objectB));
    j.insert(u"pen"_s, outlineJson(pen));
}

void writeBody(QJsonObject& j, const Equation& equation)
{
    const auto& [bounds, mathml, pointSize, color] = equation;
    j.insert(u"bounds"_s, rectJson(bounds));
    j.insert(u"mathml"_s, str(mathml)); // texte gardé tel quel, jamais interprété
    j.insert(u"pointSize"_s, pointSize);
    j.insert(u"color"_s, colorText(color));
}

// Jamais appelé : objectJson écrit un objet inconnu tel quel (std::visit exige un écrivain par type).
void writeBody(QJsonObject&, const UnknownObject&) {}

QJsonObject objectJson(const PageObject& object)
{
    // Type inconnu de cette version (L2-C8) : le JSON lu, tel quel ; ni « type » ni clés communes regénérés
    // depuis le modèle (writeCommon ajouterait les clés que le fichier n'a pas).
    if (const auto* unknown = std::get_if<UnknownObject>(&object)) {
        QJsonObject kept = keptObject(unknown->json);
        if (kept.isEmpty()) {
            // Texte gardé illisible (le lecteur n'en produit jamais) : son type seul.
            kept.insert(u"type"_s, str(unknown->type));
        }
        return kept;
    }
    QJsonObject j;
    j.insert(u"type"_s, nameText(kindOf(object)));
    writeCommon(j, object.common);
    // std::visit sur le variant de base (body), jamais sur PageObject (model.h).
    std::visit([&j](const auto& value) { writeBody(j, value); }, body(object));
    mergeKept(j, keptObject(object.unknownKeys));
    return j;
}

QJsonObject gridLevelJson(const GridLevel& level)
{
    const auto& [visible, snap, start, spacingX, spacingY, color, pattern, angle, line, linesAlongX,
                 linesAlongY, unknownKeys] = level;
    return withKept(QJsonObject{{u"visible"_s, visible},
                                {u"snap"_s, snap},
                                {u"start"_s, pointJson(start)},
                                {u"spacingX"_s, spacingX},
                                {u"spacingY"_s, spacingY},
                                {u"color"_s, colorText(color)},
                                {u"pattern"_s, nameText(pattern)},
                                {u"angle"_s, angle},
                                {u"line"_s, nameText(line)},
                                {u"linesAlongX"_s, linesAlongX},
                                {u"linesAlongY"_s, linesAlongY}},
                    unknownKeys);
}

QJsonObject gridJson(const Grid& grid)
{
    const auto& [visible, snapEnabled, drawnAbove, previewZoom, baseZoom, zoomIncrement, levels] = grid;
    QJsonArray list;
    for (const GridLevel& level : levels) {
        list.append(gridLevelJson(level));
    }
    return QJsonObject{{u"visible"_s, visible},
                       {u"snapEnabled"_s, snapEnabled},
                       {u"drawnAbove"_s, drawnAbove},
                       {u"previewZoom"_s, previewZoom},
                       {u"baseZoom"_s, baseZoom},
                       {u"zoomIncrement"_s, zoomIncrement},
                       {u"levels"_s, list}};
}

// Réglages de présentation : toutes les sous-clés, « origin » seulement si elle est mémorisée ; les sous-clés
// inconnues gardées (L2-C8) s'y ajoutent.
QJsonObject presentationJson(const PagePresentation& presentation)
{
    const auto& [tools, curtain, curtainDistance, spotlight, spotlightFrame, hasOrigin, origin, transition,
                 unknownKeys] = presentation;
    QJsonObject j{{u"tools"_s, nameText(tools)},
                  {u"curtain"_s, nameText(curtain)},
                  {u"curtainDistance"_s, curtainDistance},
                  {u"spotlight"_s, nameText(spotlight)},
                  {u"spotlightFrame"_s, rectJson(spotlightFrame)},
                  {u"transition"_s, str(transition)}};
    if (hasOrigin) {
        j.insert(u"origin"_s, pointJson(origin));
    }
    return withKept(j, unknownKeys);
}

QJsonObject pageJson(const Page& page)
{
    const auto& [width, height, objects, background, grid, name, notes, presentation, unknownKeys, baseHeight] = page; // baseHeight : L4-T6
    QJsonObject j;
    j.insert(u"width"_s, width);
    j.insert(u"height"_s, height);
    j.insert(u"name"_s, str(name));
    j.insert(u"notes"_s, str(notes));
    const auto& [fill, image] = background;
    j.insert(u"background"_s, QJsonObject{{u"fill"_s, fillJson(fill)}, {u"image"_s, str(image)}});
    j.insert(u"grid"_s, gridJson(grid));
    if (!isDefault(presentation)) {
        j.insert(u"presentation"_s, presentationJson(presentation)); // absente : réglages par défaut
    }
    QJsonArray list;
    for (const PageObject& object : objects) {
        list.append(objectJson(object));
    }
    j.insert(u"objects"_s, list);
    mergeKept(j, keptObject(unknownKeys));
    return j;
}

// --- Lecture -------------------------------------------------------------------------------------------

// 2^53 : 2^53 + 1 s'écrit 2^53 dans un réel JSON, la valeur est ambiguë ; le plus grand identifiant porté
// exactement est 2^53 − 1.
constexpr double kMaxExactId = 9007199254740992.0;

int hexDigit(QChar c)
{
    const char16_t u = c.unicode();
    if (u >= u'0' && u <= u'9') {
        return u - u'0';
    }
    if (u >= u'a' && u <= u'f') {
        return u - u'a' + 10;
    }
    if (u >= u'A' && u <= u'F') {
        return u - u'A' + 10;
    }
    return -1;
}

// Clé = SHA-256 hexadécimal minuscule ; extension comme Document::addResource. Vérifiées avant le chargeur :
// elles forment le nom d'entrée « resources/<clé>.<ext> », que « ../ » ferait viser ailleurs (écart 9 du
// plan).
bool isResourceKey(const std::string& key)
{
    return key.size() == 64 && std::all_of(key.begin(), key.end(), [](char c) {
               return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
           });
}

bool isResourceExtension(const std::string& extension)
{
    return !extension.empty() && extension.size() <= 8 &&
           std::all_of(extension.begin(), extension.end(),
                       [](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'); });
}

// Clés qu'écrit l'écrivain (L2-C8) : celles d'un objet vierge de chaque type connu, d'une page, d'une action,
// d'une ligne de texte, d'un niveau de grille, d'une déclaration de ressource et d'un document vierges
// (l'écrivain écrit toutes ses clés, quelles que soient les valeurs). Une clé lue hors de ces gabarits est
// inconnue de cette version. Construits à chaque lecture : rien de partagé entre fils.
struct KnownKeys
{
    std::array<QJsonObject, kObjectKindCount> objects; // Unknown : vide, jamais consulté
    QJsonObject page;
    QJsonObject presentation; // toutes les sous-clés, « origin » comprise
    QJsonObject action;
    QJsonObject run;
    QJsonObject level;
    QJsonObject resource;
    QJsonObject document;
};

// Membres de `read` absents de `known`, à toute profondeur d'objets ; un tableau d'objets a ses éléments
// lus un à un (leurs clés inconnues vont à l'élément du modèle), il n'est pas comparé ici. Un membre connu
// dont la valeur est mal formée n'en est pas un (le lecteur l'a signalé).
QJsonObject unknownMembers(const QJsonObject& read, const QJsonObject& known)
{
    QJsonObject out;
    for (auto it = read.begin(); it != read.end(); ++it) {
        const QJsonValue model = known.value(it.key());
        if (model.isUndefined()) {
            out.insert(it.key(), it.value());
        } else if (model.isObject() && it.value().isObject()) {
            const QJsonObject inner = unknownMembers(it.value().toObject(), model.toObject());
            if (!inner.isEmpty()) {
                out.insert(it.key(), inner);
            }
        }
    }
    return out;
}

// Texte compact des membres inconnus (unknownKeys) ; vide s'il n'y en a pas.
std::string unknownKeysOf(const QJsonObject& read, const QJsonObject& known)
{
    const QJsonObject unknown = unknownMembers(read, known);
    return unknown.isEmpty() ? std::string()
                             : QJsonDocument(unknown).toJson(QJsonDocument::Compact).toStdString();
}

// Lecture d'une valeur : absente, elle garde le défaut du modèle (sans bruit) ; présente mais mal formée
// aussi, avec un avertissement qui nomme où (`where`) et quoi.
class Reader
{
public:
    QStringList warnings;
    QString where = u"document"_s;
    const KnownKeys* known = nullptr; // gabarits des clés connues (documentFromJson)

    void warn(const QString& what) { warnings << u"%1 : %2"_s.arg(where, what); }

    QJsonObject readChild(const QJsonObject& o, QStringView key)
    {
        const QJsonValue v = o.value(key);
        if (v.isUndefined()) {
            return {};
        }
        if (!v.isObject()) {
            warn(u"« %1 » n'est pas un objet"_s.arg(key));
            return {};
        }
        return v.toObject();
    }

    QJsonArray readArray(const QJsonObject& o, QStringView key)
    {
        const QJsonValue v = o.value(key);
        if (v.isUndefined()) {
            return {};
        }
        if (!v.isArray()) {
            warn(u"« %1 » n'est pas un tableau"_s.arg(key));
            return {};
        }
        return v.toArray();
    }

    std::optional<QJsonObject> readElement(const QJsonValue& v, QStringView key)
    {
        if (!v.isObject()) {
            warn(u"« %1 » : élément qui n'est pas un objet, ignoré"_s.arg(key));
            return std::nullopt;
        }
        return v.toObject();
    }

    double readNumber(const QJsonObject& o, QStringView key, double fallback)
    {
        const QJsonValue v = o.value(key);
        if (v.isUndefined()) {
            return fallback;
        }
        if (!v.isDouble() || !std::isfinite(v.toDouble())) {
            warn(u"« %1 » n'est pas un nombre"_s.arg(key));
            return fallback;
        }
        return v.toDouble();
    }

    // Entier exact dans les bornes de T : 1e30, 2,5 ou −5 pour un non signé gardent le défaut du modèle
    // (jamais une valeur tronquée).
    template <typename T> T readInteger(const QJsonObject& o, QStringView key, T fallback)
    {
        const QJsonValue v = o.value(key);
        if (v.isUndefined()) {
            return fallback;
        }
        const double d = v.toDouble();
        if (!v.isDouble() || !std::isfinite(d) || d != std::floor(d) ||
            d < static_cast<double>(std::numeric_limits<T>::lowest()) ||
            d > static_cast<double>(std::numeric_limits<T>::max())) {
            warn(u"« %1 » n'est pas un entier dans les limites"_s.arg(key));
            return fallback;
        }
        return static_cast<T>(d);
    }

    bool readFlag(const QJsonObject& o, QStringView key, bool fallback)
    {
        const QJsonValue v = o.value(key);
        if (v.isUndefined()) {
            return fallback;
        }
        if (!v.isBool()) {
            warn(u"« %1 » n'est pas un booléen"_s.arg(key));
            return fallback;
        }
        return v.toBool();
    }

    std::string readString(const QJsonObject& o, QStringView key)
    {
        const QJsonValue v = o.value(key);
        if (v.isUndefined()) {
            return {};
        }
        if (!v.isString()) {
            warn(u"« %1 » n'est pas un texte"_s.arg(key));
            return {};
        }
        return v.toString().toStdString();
    }

    Argb readColor(const QJsonObject& o, QStringView key, Argb fallback)
    {
        const QJsonValue v = o.value(key);
        if (v.isUndefined()) {
            return fallback;
        }
        const QString s = v.toString();
        bool ok = v.isString() && s.size() == 9 && s.front() == u'#';
        Argb value = 0;
        for (qsizetype i = 1; ok && i < s.size(); ++i) {
            const int digit = hexDigit(s.at(i));
            ok = digit >= 0;
            value = (value << 4) | static_cast<Argb>(digit & 0xF);
        }
        if (!ok) {
            warn(u"« %1 » : couleur « %2 » invalide (attendu #aarrggbb)"_s.arg(key, s));
            return fallback;
        }
        return value;
    }

    template <typename E, std::size_t N>
    E readEnum(const std::array<Named<E>, N>& table, const QJsonObject& o, QStringView key, E fallback)
    {
        const QJsonValue v = o.value(key);
        if (v.isUndefined()) {
            return fallback;
        }
        if (const std::optional<E> value = valueOf(table, v.toString()); v.isString() && value) {
            return *value;
        }
        warn(u"« %1 » : valeur inconnue « %2 »"_s.arg(key, v.toString()));
        return fallback;
    }

    // Identifiant : entier de 0 à 2^53 (au-delà, un réel JSON ne le porte plus exactement) ; sinon 0, « aucun
    // ».
    ObjectId readId(const QJsonValue& v, QStringView key)
    {
        if (v.isUndefined()) {
            return 0;
        }
        const double d = v.toDouble();
        if (!v.isDouble() || !std::isfinite(d) || d < 0 || d != std::floor(d) || d >= kMaxExactId) {
            warn(u"« %1 » : identifiant invalide"_s.arg(key));
            return 0;
        }
        return static_cast<ObjectId>(d);
    }

    ObjectId readId(const QJsonObject& o, QStringView key) { return readId(o.value(key), key); }

    // Tableau de nombres à plat ; un élément non numérique devient 0 (un avertissement par tableau).
    std::vector<double> readNumbers(const QJsonValue& v, QStringView key)
    {
        std::vector<double> out;
        if (v.isUndefined()) {
            return out;
        }
        if (!v.isArray()) {
            warn(u"« %1 » n'est pas un tableau"_s.arg(key));
            return out;
        }
        const QJsonArray a = v.toArray();
        out.reserve(static_cast<std::size_t>(a.size()));
        bool clean = true;
        for (const QJsonValue& e : a) {
            const bool number = e.isDouble() && std::isfinite(e.toDouble());
            clean = clean && number;
            out.push_back(number ? e.toDouble() : 0.0);
        }
        if (!clean) {
            warn(u"« %1 » : valeur non numérique remplacée par 0"_s.arg(key));
        }
        return out;
    }

    // Points à plat (x, y, x, y…) ; un nombre impair de coordonnées perd la dernière.
    std::vector<PointF> readPairs(const QJsonValue& v, QStringView key)
    {
        const std::vector<double> n = readNumbers(v, key);
        if (n.size() % 2 != 0) {
            warn(u"« %1 » : nombre impair de coordonnées, la dernière est ignorée"_s.arg(key));
        }
        std::vector<PointF> out;
        out.reserve(n.size() / 2);
        for (std::size_t i = 0; i + 1 < n.size(); i += 2) {
            PointF point;
            auto& [x, y] = point;
            x = n[i];
            y = n[i + 1];
            out.push_back(point);
        }
        return out;
    }

    std::vector<PointF> readPairs(const QJsonObject& o, QStringView key)
    {
        return readPairs(o.value(key), key);
    }

    // Exactement N nombres finis, sinon rien (le défaut du modèle reste).
    template <std::size_t N>
    std::optional<std::array<double, N>> readFixed(const QJsonObject& o, QStringView key)
    {
        const QJsonValue v = o.value(key);
        if (v.isUndefined()) {
            return std::nullopt;
        }
        const QJsonArray a = v.toArray();
        std::array<double, N> out{};
        bool ok = v.isArray() && a.size() == static_cast<qsizetype>(N);
        for (std::size_t i = 0; ok && i < N; ++i) {
            const QJsonValue e = a.at(static_cast<qsizetype>(i));
            ok = e.isDouble() && std::isfinite(e.toDouble());
            out[i] = e.toDouble();
        }
        if (!ok) {
            warn(u"« %1 » mal formé (%2 nombres attendus)"_s.arg(key, QString::number(N)));
            return std::nullopt;
        }
        return out;
    }

    // Liaisons nommées comme ailleurs : un champ ajouté à PointF, RectF ou Transform casse la compilation.
    PointF readPoint(const QJsonObject& o, QStringView key, PointF point)
    {
        if (const std::optional<std::array<double, 2>> a = readFixed<2>(o, key)) {
            auto& [x, y] = point;
            x = (*a)[0];
            y = (*a)[1];
        }
        return point;
    }

    RectF readRect(const QJsonObject& o, QStringView key, RectF rect)
    {
        if (const std::optional<std::array<double, 4>> a = readFixed<4>(o, key)) {
            auto& [x, y, width, height] = rect;
            x = (*a)[0];
            y = (*a)[1];
            width = (*a)[2];
            height = (*a)[3];
        }
        return rect;
    }

    Transform readTransform(const QJsonObject& o, QStringView key, Transform transform)
    {
        if (const std::optional<std::array<double, 6>> a = readFixed<6>(o, key)) {
            auto& [m11, m12, m21, m22, dx, dy] = transform;
            m11 = (*a)[0];
            m12 = (*a)[1];
            m21 = (*a)[2];
            m22 = (*a)[3];
            dx = (*a)[4];
            dy = (*a)[5];
        }
        return transform;
    }

    Outline readOutline(const QJsonObject& parent, QStringView key)
    {
        const QJsonObject j = readChild(parent, key);
        Outline pen;
        auto& [style, color, width, cap, join] = pen;
        style = readEnum(kPenStyles, j, u"style", style);
        color = readColor(j, u"color", color);
        width = readNumber(j, u"width", width);
        cap = readEnum(kPenCaps, j, u"cap", cap);
        join = readEnum(kPenJoins, j, u"join", join);
        return pen;
    }

    // `brush` : le défaut du modèle à cet endroit (fond de page : blanc uni ; forme : aucun remplissage).
    Fill readFill(const QJsonObject& parent, QStringView key, Fill brush)
    {
        const QJsonObject j = readChild(parent, key);
        auto& [kind, color, color2, gradient, pattern, opaqueBackground, backgroundColor] = brush;
        kind = readEnum(kFillKinds, j, u"kind", kind);
        color = readColor(j, u"color", color);
        color2 = readColor(j, u"color2", color2);
        gradient = readEnum(kGradients, j, u"gradient", gradient);
        pattern = readInteger(j, u"pattern", pattern);
        opaqueBackground = readFlag(j, u"opaqueBackground", opaqueBackground);
        backgroundColor = readColor(j, u"backgroundColor", backgroundColor);
        return brush;
    }
};

Action readAction(Reader& r, const QJsonObject& j)
{
    Action action;
    auto& [kind, sourceType, command, target, resource, unknownKeys] = action;
    kind = r.readEnum(kActionKinds, j, u"kind", kind);
    sourceType = r.readInteger(j, u"sourceType", sourceType);
    command = r.readInteger(j, u"command", command);
    target = r.readId(j, u"target");
    resource = r.readString(j, u"resource");
    unknownKeys = unknownKeysOf(j, r.known->action);
    return action;
}

ObjectCommon readCommon(Reader& r, const QJsonObject& j)
{
    ObjectCommon common;
    auto& [id, name, layer, visible, locked, translucency, restrictions, transform, label, actions] = common;
    id = r.readId(j, u"id");
    name = r.readString(j, u"name");
    layer = r.readEnum(kLayers, j, u"layer", layer);
    visible = r.readFlag(j, u"visible", visible);
    locked = r.readFlag(j, u"locked", locked);
    translucency = r.readInteger(j, u"translucency", translucency);
    const QJsonObject jr = r.readChild(j, u"restrictions");
    auto& [movable, resizable, rotatable] = restrictions;
    movable = r.readFlag(jr, u"move", movable);
    resizable = r.readFlag(jr, u"size", resizable);
    rotatable = r.readFlag(jr, u"rotate", rotatable);
    transform = r.readTransform(j, u"transform", transform);
    label = r.readString(j, u"label");
    for (const QJsonValue& value : r.readArray(j, u"actions")) {
        if (const std::optional<QJsonObject> ja = r.readElement(value, u"actions")) {
            actions.push_back(readAction(r, *ja));
        }
    }
    return common;
}

// Un lecteur par type d'objet, en miroir de writeBody ; readObject() appelle celui du type tenu.
template <typename T> void readBody(Reader&, const QJsonObject&, T&)
{
    static_assert(sizeof(T) == 0, "type d'objet sans lecture : écrire son readBody() et son writeBody() "
                                  "(formats/src/nativejson.cpp)");
}

void readBody(Reader& r, const QJsonObject& j, Shape& shape)
{
    auto& [kind, bounds, polygon, fill, outline, outlineWidth, opacity, paths, open, windingFill, pen,
           brush, preset, parameters] = shape; // préréglage et paramètres : L4-T6
    kind = r.readEnum(kShapeKinds, j, u"shape", kind);
    bounds = r.readRect(j, u"bounds", bounds);
    polygon = r.readPairs(j, u"polygon");
    fill = r.readColor(j, u"fill", fill);
    outline = r.readColor(j, u"outline", outline);
    outlineWidth = r.readNumber(j, u"outlineWidth", outlineWidth);
    opacity = r.readNumber(j, u"opacity", opacity);
    for (const QJsonValue& subPath : r.readArray(j, u"paths")) {
        paths.push_back(r.readPairs(subPath, u"paths"));
    }
    open = r.readFlag(j, u"open", open);
    windingFill = r.readFlag(j, u"windingFill", windingFill);
    pen = r.readOutline(j, u"pen");
    brush = r.readFill(j, u"brush", brush);
}

void readBody(Reader& r, const QJsonObject& j, ImageObject& image)
{
    auto& [bounds, sourceWidth, sourceHeight, pattern, colorA, colorB, seed, resource, hasKeyColor,
           keyColor] = image;
    bounds = r.readRect(j, u"bounds", bounds);
    sourceWidth = r.readInteger(j, u"sourceWidth", sourceWidth);
    sourceHeight = r.readInteger(j, u"sourceHeight", sourceHeight);
    pattern = r.readEnum(kPatterns, j, u"pattern", pattern);
    colorA = r.readColor(j, u"colorA", colorA);
    colorB = r.readColor(j, u"colorB", colorB);
    seed = r.readInteger(j, u"seed", seed);
    resource = r.readString(j, u"resource");
    hasKeyColor = r.readFlag(j, u"hasKeyColor", hasKeyColor);
    keyColor = r.readColor(j, u"keyColor", keyColor);
}

void readBody(Reader& r, const QJsonObject& j, Stroke& stroke)
{
    auto& [points, baseWidth, color, kind, cap] = stroke;
    const std::vector<double> n = r.readNumbers(j.value(u"points"), u"points");
    if (n.size() % 3 != 0) {
        r.warn(u"« points » : triplet incomplet (x, y, pression), le dernier est ignoré"_s);
    }
    bool pressureOk = true;
    for (std::size_t i = 0; i + 2 < n.size(); i += 3) {
        StrokePoint point;
        auto& [x, y, pressure] = point;
        x = n[i];
        y = n[i + 1];
        // Hors de la plage d'un float, la conversion n'est pas définie : défaut du modèle.
        if (std::fabs(n[i + 2]) <= static_cast<double>(std::numeric_limits<float>::max())) {
            pressure = static_cast<float>(n[i + 2]);
        } else {
            pressureOk = false;
        }
        points.push_back(point);
    }
    if (!pressureOk) {
        r.warn(u"« points » : pression hors limites"_s);
    }
    baseWidth = r.readNumber(j, u"baseWidth", baseWidth);
    color = r.readColor(j, u"color", color);
    kind = r.readEnum(kStrokeKinds, j, u"kind", kind);
    cap = r.readEnum(kPenCaps, j, u"cap", cap);
}

void readBody(Reader& r, const QJsonObject& j, Line& line)
{
    auto& [points, pen] = line;
    points = r.readPairs(j, u"points");
    pen = r.readOutline(j, u"pen");
}

TextSpan readRun(Reader& r, const QJsonObject& j)
{
    TextSpan run;
    auto& [text, fontFamily, pointSize, weight, italic, underline, strikeOut, color, baseline, bounds,
           unknownKeys] = run;
    text = r.readString(j, u"text");
    fontFamily = r.readString(j, u"fontFamily");
    pointSize = r.readNumber(j, u"pointSize", pointSize);
    weight = r.readInteger(j, u"weight", weight);
    italic = r.readFlag(j, u"italic", italic);
    underline = r.readFlag(j, u"underline", underline);
    strikeOut = r.readFlag(j, u"strikeOut", strikeOut);
    color = r.readColor(j, u"color", color);
    baseline = r.readPoint(j, u"baseline", baseline);
    bounds = r.readRect(j, u"bounds", bounds);
    unknownKeys = unknownKeysOf(j, r.known->run);
    return run;
}

void readBody(Reader& r, const QJsonObject& j, Text& textObject)
{
    auto& [bounds, html, runs, vertical] = textObject; // vertical : L4-T6
    bounds = r.readRect(j, u"bounds", bounds);
    html = r.readString(j, u"html");
    for (const QJsonValue& value : r.readArray(j, u"runs")) {
        if (const std::optional<QJsonObject> jr = r.readElement(value, u"runs")) {
            runs.push_back(readRun(r, *jr));
        }
    }
}

void readBody(Reader& r, const QJsonObject& j, Equation& equation)
{
    auto& [bounds, mathml, pointSize, color] = equation;
    bounds = r.readRect(j, u"bounds", bounds);
    mathml = r.readString(j, u"mathml");
    pointSize = r.readNumber(j, u"pointSize", pointSize);
    color = r.readColor(j, u"color", color);
}

void readBody(Reader& r, const QJsonObject& j, Media& mediaObject)
{
    auto& [bounds, placeholder, media, fileName, playOnOpen, loop] = mediaObject;
    bounds = r.readRect(j, u"bounds", bounds);
    placeholder = r.readString(j, u"placeholder");
    media = r.readString(j, u"media");
    fileName = r.readString(j, u"fileName");
    playOnOpen = r.readFlag(j, u"playOnOpen", playOnOpen);
    loop = r.readFlag(j, u"loop", loop);
}

void readBody(Reader& r, const QJsonObject& j, Group& group)
{
    auto& [members] = group;
    for (const QJsonValue& value : r.readArray(j, u"members")) {
        members.push_back(r.readId(value, u"members"));
    }
}

void readBody(Reader& r, const QJsonObject& j, Connector& connector)
{
    auto& [firstEnd, secondEnd, objectA, objectB, pen] = connector;
    firstEnd = r.readPoint(j, u"firstEnd", firstEnd);
    secondEnd = r.readPoint(j, u"secondEnd", secondEnd);
    objectA = r.readId(j, u"objectA");
    objectB = r.readId(j, u"objectB");
    pen = r.readOutline(j, u"pen");
}

// Jamais appelé : readObject lit un objet inconnu par readUnknown (std::visit exige un lecteur par type).
void readBody(Reader&, const QJsonObject&, UnknownObject&) {}

// Objet vierge du type de rang `index` dans le variant (index < kObjectKindCount, garanti par kObjectKinds).
template <std::size_t I = 0> PageObject blankObject(std::size_t index)
{
    if constexpr (I + 1 < std::variant_size_v<ObjectVariant>) {
        if (index != I) {
            return blankObject<I + 1>(index);
        }
    }
    PageObject object = std::variant_alternative_t<I, ObjectVariant>{};
    return object;
}

// Boîte d'un objet inconnu, s'il en donne une lisible : quatre nombres finis, taille positive ou nulle, coins
// finis. Sans avertissement : « bounds » a le sens que lui donne un type inconnu.
std::optional<RectF> readBox(const QJsonValue& v)
{
    const QJsonArray a = v.toArray();
    if (!v.isArray() || a.size() != 4) {
        return std::nullopt;
    }
    std::array<double, 4> n{};
    for (std::size_t i = 0; i < n.size(); ++i) {
        const QJsonValue e = a.at(static_cast<qsizetype>(i));
        if (!e.isDouble() || !std::isfinite(e.toDouble())) {
            return std::nullopt;
        }
        n[i] = e.toDouble();
    }
    if (n[2] < 0 || n[3] < 0 || !std::isfinite(n[0] + n[2]) || !std::isfinite(n[1] + n[3])) {
        return std::nullopt;
    }
    RectF box;
    auto& [x, y, width, height] = box;
    x = n[0];
    y = n[1];
    width = n[2];
    height = n[3];
    return box;
}

// Objet d'un type inconnu (L2-C8) : gardé tel quel, toutes ses clés, à sa place. Ses propriétés communes sont
// lues pour la couche, l'ordre, l'identifiant et le repère, jamais réécrites d'après le modèle. Rien de ce
// qu'il cite n'est chargé.
PageObject readUnknown(Reader& r, const QJsonObject& j, const QString& type)
{
    UnknownObject unknown;
    unknown.type = type.toStdString();
    unknown.json = QJsonDocument(j).toJson(QJsonDocument::Compact).toStdString();
    if (const std::optional<RectF> box = readBox(j.value(u"bounds"_s))) {
        unknown.bounds = *box;
        unknown.hasBounds = true;
    }
    r.warn(u"objet de type inconnu « %1 » gardé tel quel (version plus récente ?) : montré par un repère, "
           u"non modifiable"_s.arg(type.left(64)));
    PageObject object = std::move(unknown);
    object.common = readCommon(r, j);
    return object;
}

std::optional<PageObject> readObject(Reader& r, const QJsonValue& value)
{
    const std::optional<QJsonObject> j = r.readElement(value, u"objects");
    if (!j) {
        return std::nullopt;
    }
    const QString type = j->value(u"type"_s).toString();
    const std::optional<ObjectKind> kind = valueOf(kObjectKinds, type);
    if (!kind) {
        return readUnknown(r, *j, type);
    }
    const auto index = static_cast<std::size_t>(*kind);
    PageObject object = blankObject(index);
    std::visit([&r, &j](auto& held) { readBody(r, *j, held); }, body(object));
    object.common = readCommon(r, *j);
    object.unknownKeys = unknownKeysOf(*j, r.known->objects[index]);
    return object;
}

GridLevel readGridLevel(Reader& r, const QJsonObject& j)
{
    GridLevel level;
    auto& [visible, snap, start, spacingX, spacingY, color, pattern, angle, line, linesAlongX, linesAlongY,
           unknownKeys] = level;
    unknownKeys = unknownKeysOf(j, r.known->level);
    visible = r.readFlag(j, u"visible", visible);
    snap = r.readFlag(j, u"snap", snap);
    start = r.readPoint(j, u"start", start);
    spacingX = r.readNumber(j, u"spacingX", spacingX);
    spacingY = r.readNumber(j, u"spacingY", spacingY);
    color = r.readColor(j, u"color", color);
    pattern = r.readEnum(kGridPatterns, j, u"pattern", pattern);
    angle = r.readNumber(j, u"angle", angle);
    line = r.readEnum(kGridLines, j, u"line", line);
    linesAlongX = r.readFlag(j, u"linesAlongX", linesAlongX);
    linesAlongY = r.readFlag(j, u"linesAlongY", linesAlongY);
    return level;
}

Grid readGrid(Reader& r, const QJsonObject& j)
{
    Grid grid;
    auto& [visible, snapEnabled, drawnAbove, previewZoom, baseZoom, zoomIncrement, levels] = grid;
    visible = r.readFlag(j, u"visible", visible);
    snapEnabled = r.readFlag(j, u"snapEnabled", snapEnabled);
    drawnAbove = r.readFlag(j, u"drawnAbove", drawnAbove);
    previewZoom = r.readNumber(j, u"previewZoom", previewZoom);
    baseZoom = r.readNumber(j, u"baseZoom", baseZoom);
    zoomIncrement = r.readNumber(j, u"zoomIncrement", zoomIncrement);
    for (const QJsonValue& value : r.readArray(j, u"levels")) {
        if (const std::optional<QJsonObject> jl = r.readElement(value, u"levels")) {
            levels.push_back(readGridLevel(r, *jl));
        }
    }
    return grid;
}

// Clé « presentation » de la page ; absente (objet vide) : réglages par défaut. Ses sous-clés inconnues
// vont à PagePresentation::unknownKeys, réécrites avec elle.
PagePresentation readPresentation(Reader& r, const QJsonObject& j)
{
    PagePresentation presentation;
    if (j.isEmpty()) {
        return presentation;
    }
    auto& [tools, curtain, curtainDistance, spotlight, spotlightFrame, hasOrigin, origin, transition,
           unknownKeys] = presentation;
    unknownKeys = unknownKeysOf(j, r.known->presentation);
    tools = r.readEnum(kToolsModes, j, u"tools", tools);
    curtain = r.readEnum(kCurtainModes, j, u"curtain", curtain);
    curtainDistance = r.readNumber(j, u"curtainDistance", curtainDistance);
    spotlight = r.readEnum(kSpotlightShapes, j, u"spotlight", spotlight);
    spotlightFrame = r.readRect(j, u"spotlightFrame", spotlightFrame);
    if (const std::optional<std::array<double, 2>> a = r.readFixed<2>(j, u"origin")) {
        hasOrigin = true;
        origin = PointF{(*a)[0], (*a)[1]};
    }
    transition = r.readString(j, u"transition");
    return presentation;
}

Page readPage(Reader& r, const QJsonObject& j)
{
    Page page;
    auto& [width, height, objects, background, grid, name, notes, presentation, unknownKeys, baseHeight] = page; // baseHeight : L4-T6
    // « presentation » est lue par readPresentation, qui garde ses sous-clés inconnues ; hors des clés
    // inconnues de la page : pageJson(Page{}) ne l'écrit pas, elle y serait gardée en double et réécrite même
    // remise aux réglages par défaut.
    QJsonObject pageKeys = j;
    pageKeys.remove(u"presentation"_s);
    unknownKeys = unknownKeysOf(pageKeys, r.known->page);
    presentation = readPresentation(r, r.readChild(j, u"presentation"));
    width = r.readNumber(j, u"width", width);
    height = r.readNumber(j, u"height", height);
    name = r.readString(j, u"name");
    notes = r.readString(j, u"notes");
    const QJsonObject jb = r.readChild(j, u"background");
    auto& [fill, image] = background;
    fill = r.readFill(jb, u"fill", fill);
    image = r.readString(jb, u"image");
    grid = readGrid(r, r.readChild(j, u"grid"));
    const QJsonArray list = r.readArray(j, u"objects");
    const QString pageWhere = r.where;
    for (qsizetype i = 0; i < list.size(); ++i) {
        r.where = u"%1, objet %2"_s.arg(pageWhere, QString::number(i + 1));
        if (std::optional<PageObject> object = readObject(r, list.at(i))) {
            objects.push_back(std::move(*object));
        }
    }
    r.where = pageWhere;
    // Page::objects est rangé par couche (model.h ; isLayerOrdered, L2-T2) : un fichier désordonné est remis
    // en ordre, stable dans chaque couche, avec un avertissement. Rien n'est perdu, d'où la tolérance.
    const auto below = [](const PageObject& a, const PageObject& b) {
        return a.common.layer < b.common.layer;
    };
    if (!std::is_sorted(objects.begin(), objects.end(), below)) {
        r.warn(u"objets hors de l'ordre des couches, remis en ordre"_s);
        std::stable_sort(objects.begin(), objects.end(), below);
    }
    return page;
}

// « actcraft-document/<majeure>[.<mineure>…] » : la majeure seule décide.
std::optional<int> majorVersion(const QString& format)
{
    static const QString prefix = u"actcraft-document/"_s;
    if (!format.startsWith(prefix)) {
        return std::nullopt;
    }
    const QString major = format.mid(prefix.size()).section(u'.', 0, 0);
    if (major.isEmpty() ||
        !std::all_of(major.begin(), major.end(), [](QChar c) { return c >= u'0' && c <= u'9'; })) {
        return std::nullopt;
    }
    bool ok = false;
    const int value = major.toInt(&ok);
    return ok ? std::optional<int>(value) : std::nullopt;
}

} // namespace

namespace {

// Entrée « resources » du JSON : clé et extension (documentToJson l'écrit, documentFromJson en déduit les
// clés connues).
QJsonObject resourceJson(const std::string& key, const std::string& extension)
{
    return QJsonObject{{u"key"_s, str(key)}, {u"extension"_s, str(extension)}};
}

} // namespace

QString describe(ResourceFailure failure)
{
    switch (failure) {
    case ResourceFailure::TooBig:
        return u"trop volumineuse"_s;
    case ResourceFailure::Corrupt:
        return u"illisible (entrée corrompue)"_s;
    case ResourceFailure::Absent:
        break;
    }
    return u"absente"_s;
}

QJsonObject documentToJson(const Document& document)
{
    QJsonObject root;
    root.insert(u"format"_s, QString::fromLatin1(kNativeFormat));
    root.insert(u"title"_s, str(document.title()));
    root.insert(u"nextId"_s, idValue(document.nextId()));
    QJsonArray resources;
    for (const auto& [key, resource] : document.resources()) {
        // Les octets vont dans l'archive (L2-T6), jamais dans le JSON.
        [[maybe_unused]] const auto& [data, extension, unknownKeys] = resource;
        resources.append(withKept(resourceJson(key, extension), unknownKeys));
    }
    root.insert(u"resources"_s, resources);
    QJsonArray pages;
    for (std::size_t i = 0; i < document.pageCount(); ++i) {
        pages.append(pageJson(document.page(i)));
    }
    root.insert(u"pages"_s, pages);
    mergeKept(root, keptObject(document.unknownKeys()));
    return root;
}

JsonReadResult documentFromJson(const QJsonObject& json, const ResourceLoader& loadResource, Document& out)
{
    JsonReadResult result;
    const QJsonValue formatValue = json.value(u"format"_s);
    const QString format = formatValue.toString();
    const std::optional<int> major = majorVersion(format);
    if (!formatValue.isString() || !format.startsWith(u"actcraft-document/"_s)) {
        result.error =
            u"format inconnu « %1 » (attendu %2)"_s.arg(format, QString::fromLatin1(kNativeFormat));
        return result;
    }
    if (!major || *major != kNativeMajorVersion) {
        result.error = u"version « %1 » du format non prise en charge (version majeure attendue : %2)"_s.arg(
            format.mid(format.indexOf(u'/') + 1), QString::number(kNativeMajorVersion));
        return result;
    }

    KnownKeys known;
    for (std::size_t i = 0; i < known.objects.size(); ++i) {
        if (i != static_cast<std::size_t>(ObjectKind::Unknown)) {
            known.objects[i] = objectJson(blankObject(i));
        }
    }
    known.page = pageJson(Page{});
    PagePresentation everyPresentationKey;
    everyPresentationKey.hasOrigin = true;
    known.presentation = presentationJson(everyPresentationKey);
    known.action = actionJson(Action{});
    known.run = runJson(TextSpan{});
    known.level = gridLevelJson(GridLevel{});
    known.resource = resourceJson({}, {});
    known.document = documentToJson(Document{});

    // Tout est lu dans un document neuf : `out` n'est remplacé qu'à la fin, lecture réussie.
    Reader r;
    r.known = &known;
    std::vector<Page> pages;
    const QJsonArray list = r.readArray(json, u"pages");
    for (qsizetype i = 0; i < list.size(); ++i) {
        r.where = u"page %1"_s.arg(i + 1);
        if (const std::optional<QJsonObject> jp = r.readElement(list.at(i), u"pages")) {
            pages.push_back(readPage(r, *jp));
        }
    }
    r.where = u"document"_s;
    if (pages.empty()) {
        r.warn(u"aucune page : une page vierge est créée"_s);
    }
    Document document(std::move(pages));
    document.setTitle(r.readString(json, u"title"));
    document.setUnknownKeys(unknownKeysOf(json, known.document));
    for (const QJsonValue& value : r.readArray(json, u"resources")) {
        const std::optional<QJsonObject> jr = r.readElement(value, u"resources");
        if (!jr) {
            continue;
        }
        const std::string key = jr->value(u"key"_s).toString().toStdString();
        const std::string extension = jr->value(u"extension"_s).toString().toStdString();
        if (!isResourceKey(key) || !isResourceExtension(extension)) {
            r.warn(u"ressource « %1.%2 » : nom invalide, écartée"_s.arg(str(key), str(extension)));
            continue;
        }
        if (document.resource(key)) {
            r.warn(u"ressource %1 déclarée deux fois"_s.arg(str(key)));
            continue;
        }
        const LoadedResource loaded =
            loadResource ? loadResource(key, extension) : LoadedResource(std::nullopt);
        const std::optional<QByteArray>& data = loaded.data;
        if (!data) {
            // Cause typée rendue par le chargeur (L2-C9) : le texte n'est écrit qu'ici.
            r.warn(u"ressource %1 %2, écartée"_s.arg(str(key), describe(loaded.failure)));
            continue;
        }
        if (sha256Hex(*data) != key) {
            r.warn(u"ressource %1 : empreinte différente, écartée"_s.arg(str(key)));
            continue;
        }
        document.addResource(*data, extension);
        document.setResourceUnknownKeys(key, unknownKeysOf(*jr, known.resource));
    }
    document.reserveIdsBelow(r.readId(json, u"nextId"));
    if (!hasUniqueIds(document)) {
        r.warn(u"identifiants d'objets absents ou en double"_s);
    }
    out = std::move(document);
    result.ok = true;
    result.warnings = std::move(r.warnings);
    return result;
}

} // namespace actcraft::formats
