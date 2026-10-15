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
    void f64(double v) noexcept { u64(std::bit_cast<std::uint64_t>(v)); }
    void f32(float v) noexcept { u32(std::bit_cast<std::uint32_t>(v)); }
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
    const auto& [canMove, canSize, canRotate] = restrictions;
    h.boolean(canMove);
    h.boolean(canSize);
    h.boolean(canRotate);
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

void hashBody(Fnv1a& h, const PageObject& object) noexcept
{
    if (const auto* s = std::get_if<Shape>(&object)) {
        const auto& [kind, bounds, polygon, fill, outline, outlineWidth, opacity, paths, open, windingFill,
                     pen, brush] = *s;
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
    } else if (const auto* im = std::get_if<ImageObject>(&object)) {
        const auto& [bounds, sourceWidth, sourceHeight, pattern, colorA, colorB, seed, resource,
                     hasTransparentColor, transparentColor] = *im;
        h.rect(bounds);
        h.i32(sourceWidth);
        h.i32(sourceHeight);
        h.tag(pattern);
        h.u32(colorA);
        h.u32(colorB);
        h.u32(seed);
        h.text(resource);
        h.boolean(hasTransparentColor);
        h.u32(transparentColor);
    } else if (const auto* st = std::get_if<Stroke>(&object)) {
        const auto& [points, baseWidth, color, kind, cap] = *st;
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
    } else if (const auto* l = std::get_if<Line>(&object)) {
        const auto& [points, pen] = *l;
        h.points(points);
        h.outline(pen);
    } else if (const auto* t = std::get_if<Text>(&object)) {
        const auto& [bounds, html, runs] = *t;
        h.rect(bounds);
        h.text(html);
        h.u64(static_cast<std::uint64_t>(runs.size()));
        for (const TextRun& r : runs) {
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
    } else if (const auto* m = std::get_if<Media>(&object)) {
        const auto& [bounds, placeholder, media, fileName, autoPlay, loop] = *m;
        h.rect(bounds);
        h.text(placeholder);
        h.text(media);
        h.text(fileName);
        h.boolean(autoPlay);
        h.boolean(loop);
    } else if (const auto* g = std::get_if<Group>(&object)) {
        const auto& [members] = *g;
        h.u64(static_cast<std::uint64_t>(members.size()));
        for (const ObjectId id : members) {
            h.u64(id);
        }
    } else if (const auto* c = std::get_if<Connector>(&object)) {
        const auto& [endA, endB, objectA, objectB, pen] = *c;
        h.point(endA);
        h.point(endB);
        h.u64(objectA);
        h.u64(objectB);
        h.outline(pen);
    }
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
              "type d'objet ajouté : l'ajouter à hashBody() et à kObjectKindCount");

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
    const auto& [visible, allowSnap, onTop, thumbnailScale, defaultScale, scaleStep, levels] = grid;
    h.boolean(visible);
    h.boolean(allowSnap);
    h.boolean(onTop);
    h.f64(thumbnailScale);
    h.f64(defaultScale);
    h.f64(scaleStep);
    h.u64(static_cast<std::uint64_t>(levels.size()));
    for (const GridLevel& l : levels) {
        const auto& [lvVisible, snap, start, stepX, stepY, color, pattern, angle, line, drawX, drawY] = l;
        h.boolean(lvVisible);
        h.boolean(snap);
        h.point(start);
        h.f64(stepX);
        h.f64(stepY);
        h.u32(color);
        h.tag(pattern);
        h.f64(angle);
        h.tag(line);
        h.boolean(drawX);
        h.boolean(drawY);
    }
    h.text(name);
    h.text(notes);
    return h.value();
}

} // namespace actcraft::core
