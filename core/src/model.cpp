#include <actcraft/core/model.h>

#include <bit>
#include <variant>

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
    void point(const PointF& p) noexcept
    {
        const auto& [x, y] = p;
        f64(x);
        f64(y);
    }
    void rect(const RectF& r) noexcept
    {
        const auto& [x, y, width, height] = r;
        f64(x);
        f64(y);
        f64(width);
        f64(height);
    }
    std::uint64_t value() const noexcept { return m_hash; }

private:
    std::uint64_t m_hash = 0xCBF29CE484222325ull;
};

} // namespace

ObjectKind kindOf(const PageObject& object) noexcept
{
    return static_cast<ObjectKind>(object.index());
}

// Chaque structure est lue par une liaison structurée qui nomme tous ses champs : un champ ajouté casse la
// compilation au lieu d'échapper à l'empreinte. Champs hachés dans l'ordre de déclaration (ne pas le changer
// : l'empreinte figée en dépend).
static_assert(std::variant_size_v<PageObject> == 3, "type d'objet ajouté : l'ajouter à fingerprint()");

std::uint64_t fingerprint(const Page& page) noexcept
{
    const auto& [width, height, objects] = page;
    Fnv1a h;
    h.f64(width);
    h.f64(height);
    h.u64(static_cast<std::uint64_t>(objects.size()));
    for (const PageObject& object : objects) {
        h.byte(static_cast<std::uint8_t>(kindOf(object)));
        if (const auto* s = std::get_if<Shape>(&object)) {
            const auto& [kind, bounds, polygon, fill, outline, outlineWidth, opacity] = *s;
            h.byte(static_cast<std::uint8_t>(kind));
            h.rect(bounds);
            h.u64(static_cast<std::uint64_t>(polygon.size()));
            for (const PointF& p : polygon) {
                h.point(p);
            }
            h.u32(fill);
            h.u32(outline);
            h.f64(outlineWidth);
            h.f64(opacity);
        } else if (const auto* im = std::get_if<ImageObject>(&object)) {
            const auto& [bounds, sourceWidth, sourceHeight, pattern, colorA, colorB, seed] = *im;
            h.rect(bounds);
            h.i32(sourceWidth);
            h.i32(sourceHeight);
            h.byte(static_cast<std::uint8_t>(pattern));
            h.u32(colorA);
            h.u32(colorB);
            h.u32(seed);
        } else if (const auto* st = std::get_if<Stroke>(&object)) {
            const auto& [points, baseWidth, color] = *st;
            h.u64(static_cast<std::uint64_t>(points.size()));
            for (const StrokePoint& p : points) {
                const auto& [x, y, pressure] = p;
                h.f64(x);
                h.f64(y);
                h.f32(pressure);
            }
            h.f64(baseWidth);
            h.u32(color);
        }
    }
    return h.value();
}

} // namespace actcraft::core
