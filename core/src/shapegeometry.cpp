#include <actcraft/core/shapegeometry.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

namespace actcraft::core {

namespace {

constexpr double kPi = std::numbers::pi;
constexpr int kEllipseVertices = 64;
constexpr int kCornerVertices = 8;
constexpr double kCalloutTipDepth = 1.3;  // la pointe reste sous la boîte, à 1,3 de la hauteur (non exposé)
constexpr double kCalloutBaseWidth = 0.2; // largeur de la base de la pointe, fraction de la largeur

ShapePreset makePreset(const char* id, ShapeFamily family, bool square, int axis,
                       std::vector<double> defaults, std::vector<double> minimum, std::vector<double> maximum)
{
    ShapePreset preset;
    preset.id = id;
    preset.family = family;
    preset.square = square;
    preset.axis = axis;
    preset.defaults = std::move(defaults);
    preset.minimum = std::move(minimum);
    preset.maximum = std::move(maximum);
    return preset;
}

ShapePreset linePreset(const char* id, int axis)
{
    return makePreset(id, ShapeFamily::Line, false, axis, {}, {}, {});
}

ShapePreset closedPreset(const char* id, bool square = false)
{
    return makePreset(id, ShapeFamily::Closed, square, 0, {}, {}, {});
}

std::vector<ShapePreset> buildCatalog()
{
    std::vector<ShapePreset> catalog;
    catalog.reserve(22);
    // Ordre de la palette (D1, amendement 4).
    catalog.push_back(linePreset("line", 0));
    catalog.push_back(linePreset("line-horizontal", 1));
    catalog.push_back(linePreset("line-vertical", 2));
    catalog.push_back(linePreset("freeform", 0));
    catalog.push_back(linePreset("polyline", 0));
    catalog.push_back(linePreset("curve-chain", 0));
    catalog.push_back(closedPreset("rectangle"));
    catalog.push_back(makePreset("rounded-rectangle", ShapeFamily::Closed, false, 0, {0.15}, {0.01}, {0.5}));
    catalog.push_back(closedPreset("square", true));
    catalog.push_back(closedPreset("ellipse"));
    catalog.push_back(closedPreset("circle", true));
    catalog.push_back(closedPreset("triangle"));
    catalog.push_back(closedPreset("diamond"));
    catalog.push_back(closedPreset("pentagon"));
    catalog.push_back(closedPreset("hexagon"));
    catalog.push_back(closedPreset("star"));
    catalog.push_back(
        makePreset("arrow", ShapeFamily::Closed, false, 0, {0.6, 0.25}, {0.0, 0.0}, {1.0, 0.5}));
    catalog.push_back(
        makePreset("callout", ShapeFamily::Closed, false, 0, {1.0, 0.1}, {0.0, 0.01}, {1.0, 0.3}));
    catalog.push_back(closedPreset("octagon"));
    catalog.push_back(closedPreset("star-four"));
    catalog.push_back(closedPreset("burst"));
    catalog.push_back(closedPreset("burst-twelve"));
    return catalog;
}

double clampTo(double value, double low, double high) noexcept
{
    return std::min(std::max(value, low), high);
}

// Paramètres effectifs : ceux de la forme, complétés par les valeurs par défaut, bornés.
std::vector<double> effectiveParameters(const ShapePreset& preset, const std::vector<double>& given)
{
    std::vector<double> out = preset.defaults;
    for (std::size_t i = 0; i < out.size(); ++i) {
        if (i < given.size())
            out[i] = given[i];
        out[i] = clampTo(out[i], preset.minimum[i], preset.maximum[i]);
    }
    return out;
}

void addArc(std::vector<PointF>& out, double cx, double cy, double radius, double fromDegrees,
            double toDegrees)
{
    for (int i = 0; i < kCornerVertices; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(kCornerVertices - 1);
        const double angle = (fromDegrees + (toDegrees - fromDegrees) * t) * kPi / 180.0;
        out.push_back({cx + radius * std::cos(angle), cy + radius * std::sin(angle)});
    }
}

// Rectangle arrondi de la boîte {0, 0, w, h}, sens horaire ; `tipX` insère la pointe d'une bulle sur le bord
// bas.
std::vector<PointF> roundedBox(double w, double h, double radius, const double* tipX)
{
    std::vector<PointF> out;
    addArc(out, w - radius, radius, radius, -90, 0);
    addArc(out, w - radius, h - radius, radius, 0, 90);
    if (tipX != nullptr) {
        const double half = kCalloutBaseWidth * w / 2;
        const double low = radius + half;
        const double high = std::max(low, w - radius - half);
        const double base = clampTo(*tipX, low, high);
        out.push_back({base + half, h});
        out.push_back({*tipX, h * kCalloutTipDepth});
        out.push_back({base - half, h});
    }
    addArc(out, radius, h - radius, radius, 90, 180);
    addArc(out, radius, radius, radius, 180, 270);
    return out;
}

// Polygone régulier ou étoile inscrit dans l'ellipse de la boîte, premier sommet en haut.
std::vector<PointF> radial(double w, double h, int sides, double innerRatio)
{
    std::vector<PointF> out;
    const double cx = w / 2;
    const double cy = h / 2;
    const int count = innerRatio > 0 ? sides * 2 : sides;
    for (int i = 0; i < count; ++i) {
        const double ratio = (innerRatio > 0 && i % 2 == 1) ? innerRatio : 1.0;
        const double angle = -kPi / 2 + 2 * kPi * static_cast<double>(i) / static_cast<double>(count);
        out.push_back({cx + ratio * (w / 2) * std::cos(angle), cy + ratio * (h / 2) * std::sin(angle)});
    }
    return out;
}

std::vector<PointF> buildPolygon(const ShapePreset& preset, double w, double h, const std::vector<double>& p)
{
    const std::string& id = preset.id;
    const double smallest = std::min(w, h);
    if (id == "rounded-rectangle")
        return roundedBox(w, h, p[0] * smallest, nullptr);
    if (id == "callout") {
        const double tip = p[0] * w;
        return roundedBox(w, h, p[1] * smallest, &tip);
    }
    if (id == "ellipse" || id == "circle")
        return radial(w, h, kEllipseVertices, 0);
    if (id == "triangle")
        return {{w / 2, 0}, {w, h}, {0, h}};
    if (id == "diamond")
        return {{w / 2, 0}, {w, h / 2}, {w / 2, h}, {0, h / 2}};
    if (id == "pentagon")
        return radial(w, h, 5, 0);
    if (id == "hexagon")
        return {{w / 4, 0}, {3 * w / 4, 0}, {w, h / 2}, {3 * w / 4, h}, {w / 4, h}, {0, h / 2}};
    if (id == "octagon") {
        const double c = 0.29 * smallest;
        return {{c, 0}, {w - c, 0}, {w, c}, {w, h - c}, {w - c, h}, {c, h}, {0, h - c}, {0, c}};
    }
    if (id == "star")
        return radial(w, h, 5, 0.45);
    if (id == "star-four")
        return radial(w, h, 4, 0.35);
    if (id == "burst")
        return radial(w, h, 8, 0.6);
    if (id == "burst-twelve")
        return radial(w, h, 12, 0.7);
    if (id == "arrow") {
        const double headX = p[0] * w;
        const double top = (h - p[1] * h) / 2;
        const double bottom = top + p[1] * h;
        return {{0, top}, {headX, top}, {headX, 0}, {w, h / 2}, {headX, h}, {headX, bottom}, {0, bottom}};
    }
    // rectangle, square et tout préréglage fermé sans géométrie propre
    return {{0, 0}, {w, 0}, {w, h}, {0, h}};
}

Outline outlineOf(const ShapeStyle& style)
{
    Outline pen;
    pen.style = PenStyle::Solid;
    pen.color = style.outline;
    pen.width = style.width;
    return pen;
}

} // namespace

const std::vector<ShapePreset>& shapeCatalog()
{
    static const std::vector<ShapePreset> catalog = buildCatalog();
    return catalog;
}

const ShapePreset* findShapePreset(std::string_view id) noexcept
{
    for (const auto& preset : shapeCatalog()) {
        if (preset.id == id)
            return &preset;
    }
    return nullptr;
}

RectF dragBox(PointF press, PointF current, bool fromCenter, bool square) noexcept
{
    const double dx = current.x - press.x;
    const double dy = current.y - press.y;
    double halfX = std::abs(dx);
    double halfY = std::abs(dy);
    if (square) {
        halfX = std::max(halfX, halfY);
        halfY = halfX;
    }
    if (fromCenter)
        return {press.x - halfX, press.y - halfY, 2 * halfX, 2 * halfY};
    const double x = dx >= 0 ? press.x : press.x - halfX;
    const double y = dy >= 0 ? press.y : press.y - halfY;
    return {x, y, halfX, halfY};
}

std::pair<PointF, PointF> dragLine(PointF press, PointF current, const ShapePreset& preset, bool fromCenter,
                                   bool constrain45) noexcept
{
    PointF to = current;
    if (preset.axis == 1) {
        to.y = press.y;
    } else if (preset.axis == 2) {
        to.x = press.x;
    } else if (constrain45) {
        const double dx = current.x - press.x;
        const double dy = current.y - press.y;
        const double length = std::hypot(dx, dy);
        const double step = kPi / 4;
        const double angle = std::round(std::atan2(dy, dx) / step) * step;
        to = {press.x + length * std::cos(angle), press.y + length * std::sin(angle)};
    }
    const PointF from = fromCenter ? PointF{2 * press.x - to.x, 2 * press.y - to.y} : press;
    return {from, to};
}

PageObject makeShapeObject(std::string_view preset, const RectF& box, const ShapeStyle& style)
{
    const ShapePreset* found = findShapePreset(preset);
    if (found == nullptr || found->family != ShapeFamily::Closed)
        found = findShapePreset("rectangle");
    Shape shape;
    shape.kind = ShapeKind::Polygon;
    shape.bounds = {0, 0, box.width, box.height};
    shape.outline = style.outline;
    shape.outlineWidth = style.width;
    shape.fill = style.filled ? style.fill : 0u;
    shape.pen = outlineOf(style);
    shape.brush.kind = style.filled ? FillKind::Solid : FillKind::None;
    shape.brush.color = style.fill;
    shape.preset = found->id;
    shape.parameters = found->defaults;
    rebuildShape(shape);
    PageObject object{std::move(shape)};
    object.common.layer = Layer::Middle;
    object.common.transform = translation(box.x, box.y);
    return object;
}

PageObject makeLineObject(std::string_view preset, PointF from, PointF to, const ShapeStyle& style)
{
    static_cast<void>(preset); // toute la famille : un segment (freeform, polyline, curve-chain : L4-T31)
    Line line;
    line.points = {from, to};
    line.pen = outlineOf(style);
    PageObject object{std::move(line)};
    object.common.layer = Layer::Middle;
    return object;
}

bool rebuildShape(Shape& shape)
{
    const ShapePreset* preset = findShapePreset(shape.preset);
    if (preset == nullptr || preset->family != ShapeFamily::Closed)
        return false;
    const std::vector<double> params = effectiveParameters(*preset, shape.parameters);
    shape.kind = ShapeKind::Polygon;
    shape.polygon = buildPolygon(*preset, shape.bounds.width, shape.bounds.height, params);
    shape.paths.clear();
    return true;
}

std::vector<PointF> smartPoints(const Shape& shape)
{
    const ShapePreset* preset = findShapePreset(shape.preset);
    if (preset == nullptr || preset->defaults.empty())
        return {};
    const std::vector<double> p = effectiveParameters(*preset, shape.parameters);
    const double w = shape.bounds.width;
    const double h = shape.bounds.height;
    const double smallest = std::min(w, h);
    if (preset->id == "rounded-rectangle")
        return {{p[0] * smallest, 0}};
    if (preset->id == "arrow")
        return {{p[0] * w, 0}, {p[0] * w / 2, (h - p[1] * h) / 2}};
    if (preset->id == "callout")
        return {{p[0] * w, h * kCalloutTipDepth}, {p[1] * smallest, 0}};
    return {};
}

std::vector<double> dragSmartPoint(const Shape& shape, std::size_t index, PointF objectPoint)
{
    const ShapePreset* preset = findShapePreset(shape.preset);
    if (preset == nullptr || preset->defaults.empty())
        return {};
    std::vector<double> p = effectiveParameters(*preset, shape.parameters);
    if (index >= p.size())
        return p;
    const double w = shape.bounds.width;
    const double h = shape.bounds.height;
    const double smallest = std::min(w, h);
    const auto ratio = [](double value, double whole) { return whole > 0 ? value / whole : 0.0; };
    double value = p[index];
    if (preset->id == "rounded-rectangle") {
        value = ratio(objectPoint.x, smallest);
    } else if (preset->id == "arrow") {
        value = index == 0 ? ratio(objectPoint.x, w) : 1.0 - 2.0 * ratio(objectPoint.y, h);
    } else if (preset->id == "callout") {
        value = index == 0 ? ratio(objectPoint.x, w) : ratio(objectPoint.x, smallest);
    }
    p[index] = clampTo(value, preset->minimum[index], preset->maximum[index]);
    return p;
}

ShapeGesture shapeGesture(std::string_view preset) noexcept
{
    if (preset == "freeform")
        return ShapeGesture::Freehand;
    if (preset == "polyline" || preset == "curve-chain")
        return ShapeGesture::Clicks;
    return ShapeGesture::Box;
}

} // namespace actcraft::core
