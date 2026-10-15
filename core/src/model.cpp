#include <actcraft/core/model.h>

#include <bit>
#include <cmath>
#include <numbers>

namespace actcraft::core {

namespace {

class Fnv1a
{
public:
    void byte(std::uint8_t b) noexcept
    {
        m_hash ^= b;
        m_hash *= 0x100000001B3ull;
    }
    void u32(std::uint32_t v) noexcept
    {
        for (unsigned i = 0; i < 4; ++i) {
            byte(static_cast<std::uint8_t>((v >> (8u * i)) & 0xFFu));
        }
    }
    void u64(std::uint64_t v) noexcept
    {
        for (unsigned i = 0; i < 8; ++i) {
            byte(static_cast<std::uint8_t>((v >> (8u * i)) & 0xFFu));
        }
    }
    void i32(std::int32_t v) noexcept { u32(static_cast<std::uint32_t>(v)); }
    // −0 et +0 se valent : on hache +0 pour les deux (le −0 ne survit pas à un aller-retour JSON). Un NaN
    // garde son motif binaire.
    void f64(double v) noexcept { u64(std::bit_cast<std::uint64_t>(v == 0.0 ? 0.0 : v)); }
    void f32(float v) noexcept { u32(std::bit_cast<std::uint32_t>(v == 0.0f ? 0.0f : v)); }
    void boolean(bool v) noexcept { byte(v ? 1u : 0u); }
    template <typename E> void tag(E v) noexcept { byte(static_cast<std::uint8_t>(v)); }
    void text(const std::string& s) noexcept
    {
        u64(static_cast<std::uint64_t>(s.size()));
        for (const char c : s) {
            byte(static_cast<std::uint8_t>(c));
        }
    }
    void point(const PointF& p) noexcept
    {
        const auto& [x, y] = p;
        f64(x);
        f64(y);
    }
    void points(const std::vector<PointF>& ps) noexcept
    {
        u64(static_cast<std::uint64_t>(ps.size()));
        for (const PointF& p : ps) {
            point(p);
        }
    }
    void rect(const RectF& r) noexcept
    {
        const auto& [x, y, width, height] = r;
        f64(x);
        f64(y);
        f64(width);
        f64(height);
    }
    void transform(const Transform& t) noexcept
    {
        const auto& [m11, m12, m21, m22, dx, dy] = t;
        f64(m11);
        f64(m12);
        f64(m21);
        f64(m22);
        f64(dx);
        f64(dy);
    }
    void outline(const Outline& o) noexcept
    {
        const auto& [style, color, width, cap, join] = o;
        tag(style);
        u32(color);
        f64(width);
        tag(cap);
        tag(join);
    }
    void fill(const Fill& f) noexcept
    {
        const auto& [kind, color, color2, gradient, pattern, opaqueBackground, backgroundColor] = f;
        tag(kind);
        u32(color);
        u32(color2);
        tag(gradient);
        byte(pattern);
        boolean(opaqueBackground);
        u32(backgroundColor);
    }
    std::uint64_t value() const noexcept { return m_hash; }

private:
    std::uint64_t m_hash = 0xCBF29CE484222325ull;
};

void hashCommon(Fnv1a& h, const ObjectCommon& c) noexcept
{
    const auto& [id, name, layer, visible, locked, translucency, restrictions, transform, label, actions] = c;
    h.u64(id);
    h.text(name);
    h.tag(layer);
    h.boolean(visible);
    h.boolean(locked);
    h.byte(translucency);
    const auto& [movable, resizable, rotatable] = restrictions;
    h.boolean(movable);
    h.boolean(resizable);
    h.boolean(rotatable);
    h.transform(transform);
    h.text(label);
    h.u64(static_cast<std::uint64_t>(actions.size()));
    for (const Action& a : actions) {
        const auto& [kind, sourceType, command, target, resource] = a;
        h.tag(kind);
        h.u32(sourceType);
        h.i32(command);
        h.u64(target);
        h.text(resource);
    }
}

// Un hachage par type d'objet ; hashBody() appelle celui du type tenu. Un type ajouté au variant sans sa
// fonction tombe sur ce modèle et la compilation échoue (relecture de L2-T1 : une chaîne de get_if sans
// branche finale laissait un type nouveau hors de l'empreinte, deux pages différentes ayant alors la même).
template <typename T> void hashType(Fnv1a&, const T&) noexcept
{
    static_assert(sizeof(T) == 0,
                  "type d'objet sans hachage : écrire son hashType() (et suivre le script de L2-T1)");
}

void hashType(Fnv1a& h, const Shape& shape) noexcept
{
    const auto& [kind, bounds, polygon, fill, outline, outlineWidth, opacity, paths, open, windingFill, pen,
                 brush] = shape;
    h.tag(kind);
    h.rect(bounds);
    h.points(polygon);
    h.u32(fill);
    h.u32(outline);
    h.f64(outlineWidth);
    h.f64(opacity);
    h.u64(static_cast<std::uint64_t>(paths.size()));
    for (const auto& path : paths) {
        h.points(path);
    }
    h.boolean(open);
    h.boolean(windingFill);
    h.outline(pen);
    h.fill(brush);
}

void hashType(Fnv1a& h, const ImageObject& image) noexcept
{
    const auto& [bounds, sourceWidth, sourceHeight, pattern, colorA, colorB, seed, resource, hasKeyColor,
                 keyColor] = image;
    h.rect(bounds);
    h.i32(sourceWidth);
    h.i32(sourceHeight);
    h.tag(pattern);
    h.u32(colorA);
    h.u32(colorB);
    h.u32(seed);
    h.text(resource);
    h.boolean(hasKeyColor);
    h.u32(keyColor);
}

void hashType(Fnv1a& h, const Stroke& stroke) noexcept
{
    const auto& [points, baseWidth, color, kind, cap] = stroke;
    h.u64(static_cast<std::uint64_t>(points.size()));
    for (const StrokePoint& p : points) {
        const auto& [x, y, pressure] = p;
        h.f64(x);
        h.f64(y);
        h.f32(pressure);
    }
    h.f64(baseWidth);
    h.u32(color);
    h.tag(kind);
    h.tag(cap);
}

void hashType(Fnv1a& h, const Line& line) noexcept
{
    const auto& [points, pen] = line;
    h.points(points);
    h.outline(pen);
}

void hashType(Fnv1a& h, const Text& textObject) noexcept
{
    const auto& [bounds, html, runs] = textObject;
    h.rect(bounds);
    h.text(html);
    h.u64(static_cast<std::uint64_t>(runs.size()));
    for (const TextSpan& r : runs) {
        const auto& [text, fontFamily, pointSize, weight, italic, underline, strikeOut, color, baseline,
                     runBounds] = r;
        h.text(text);
        h.text(fontFamily);
        h.f64(pointSize);
        h.i32(weight);
        h.boolean(italic);
        h.boolean(underline);
        h.boolean(strikeOut);
        h.u32(color);
        h.point(baseline);
        h.rect(runBounds);
    }
}

void hashType(Fnv1a& h, const Media& mediaObject) noexcept
{
    const auto& [bounds, placeholder, media, fileName, playOnOpen, loop] = mediaObject;
    h.rect(bounds);
    h.text(placeholder);
    h.text(media);
    h.text(fileName);
    h.boolean(playOnOpen);
    h.boolean(loop);
}

void hashType(Fnv1a& h, const Group& group) noexcept
{
    const auto& [members] = group;
    h.u64(static_cast<std::uint64_t>(members.size()));
    for (const ObjectId id : members) {
        h.u64(id);
    }
}

void hashType(Fnv1a& h, const Connector& connector) noexcept
{
    const auto& [firstEnd, secondEnd, objectA, objectB, pen] = connector;
    h.point(firstEnd);
    h.point(secondEnd);
    h.u64(objectA);
    h.u64(objectB);
    h.outline(pen);
}

// std::visit sur le variant de base (body), jamais sur PageObject : il couvre chaque type par construction.
void hashBody(Fnv1a& h, const PageObject& object) noexcept
{
    std::visit([&h](const auto& value) { hashType(h, value); }, body(object));
}

} // namespace

Transform compose(const Transform& a, const Transform& b) noexcept
{
    Transform t;
    t.m11 = a.m11 * b.m11 + a.m12 * b.m21;
    t.m12 = a.m11 * b.m12 + a.m12 * b.m22;
    t.m21 = a.m21 * b.m11 + a.m22 * b.m21;
    t.m22 = a.m21 * b.m12 + a.m22 * b.m22;
    t.dx = a.dx * b.m11 + a.dy * b.m21 + b.dx;
    t.dy = a.dx * b.m12 + a.dy * b.m22 + b.dy;
    return t;
}

Transform translation(double dx, double dy) noexcept
{
    Transform t;
    t.dx = dx;
    t.dy = dy;
    return t;
}

Transform scaling(double sx, double sy) noexcept
{
    Transform t;
    t.m11 = sx;
    t.m22 = sy;
    return t;
}

Transform rotation(double degrees) noexcept
{
    double turn = std::fmod(degrees, 360.0);
    if (turn < 0) {
        turn += 360.0;
    }
    double c = 1;
    double s = 0;
    if (turn == 90) {
        c = 0;
        s = 1;
    } else if (turn == 180) {
        c = -1;
        s = 0;
    } else if (turn == 270) {
        c = 0;
        s = -1;
    } else if (turn != 0) {
        const double r = turn * std::numbers::pi / 180.0;
        c = std::cos(r);
        s = std::sin(r);
    }
    Transform t;
    t.m11 = c;
    t.m12 = s;
    t.m21 = 0.0 - s; // pas -s : -0 pour 0° et 180°, motif binaire différent dans l'empreinte
    t.m22 = c;
    return t;
}

ObjectKind kindOf(const PageObject& object) noexcept
{
    return static_cast<ObjectKind>(object.index());
}

// Chaque structure est lue par une liaison structurée qui nomme tous ses champs : un champ ajouté casse la
// compilation au lieu d'échapper à l'empreinte. Champs hachés dans l'ordre de déclaration ; pour chaque objet
// : son type (un octet), ses propriétés communes, puis les champs de son type. Le script Python de L2-T1
// (étape 7) suit ce même ordre : le modifier change les valeurs figées des tests.
static_assert(std::variant_size_v<ObjectVariant> == kObjectKindCount,
              "type d'objet ajouté : l'ajouter à kObjectKindCount (hashType() : la compilation l'exige)");

std::uint64_t fingerprint(const Page& page) noexcept
{
    const auto& [width, height, objects, background, grid, name, notes] = page;
    Fnv1a h;
    h.f64(width);
    h.f64(height);
    h.u64(static_cast<std::uint64_t>(objects.size()));
    for (const PageObject& object : objects) {
        h.tag(kindOf(object));
        hashCommon(h, object.common);
        hashBody(h, object);
    }
    const auto& [fill, image] = background;
    h.fill(fill);
    h.text(image);
    const auto& [visible, snapEnabled, drawnAbove, previewZoom, baseZoom, zoomIncrement, levels] = grid;
    h.boolean(visible);
    h.boolean(snapEnabled);
    h.boolean(drawnAbove);
    h.f64(previewZoom);
    h.f64(baseZoom);
    h.f64(zoomIncrement);
    h.u64(static_cast<std::uint64_t>(levels.size()));
    for (const GridLevel& l : levels) {
        const auto& [lvVisible, snap, start, spacingX, spacingY, color, pattern, angle, line, linesAlongX,
                     linesAlongY] = l;
        h.boolean(lvVisible);
        h.boolean(snap);
        h.point(start);
        h.f64(spacingX);
        h.f64(spacingY);
        h.u32(color);
        h.tag(pattern);
        h.f64(angle);
        h.tag(line);
        h.boolean(linesAlongX);
        h.boolean(linesAlongY);
    }
    h.text(name);
    h.text(notes);
    return h.value();
}

} // namespace actcraft::core
