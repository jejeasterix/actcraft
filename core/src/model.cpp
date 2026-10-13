#include <actcraft/core/model.h>

#include <bit>

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
        f64(p.x);
        f64(p.y);
    }
    void rect(const RectF& r) noexcept
    {
        f64(r.x);
        f64(r.y);
        f64(r.width);
        f64(r.height);
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

std::uint64_t fingerprint(const Page& page) noexcept
{
    Fnv1a h;
    h.f64(page.width);
    h.f64(page.height);
    h.u64(static_cast<std::uint64_t>(page.objects.size()));
    for (const PageObject& object : page.objects) {
        h.byte(static_cast<std::uint8_t>(kindOf(object)));
        if (const auto* s = std::get_if<Shape>(&object)) {
            h.byte(static_cast<std::uint8_t>(s->kind));
            h.rect(s->bounds);
            h.u64(static_cast<std::uint64_t>(s->polygon.size()));
            for (const PointF& p : s->polygon) {
                h.point(p);
            }
            h.u32(s->fill);
            h.u32(s->outline);
            h.f64(s->outlineWidth);
            h.f64(s->opacity);
        } else if (const auto* im = std::get_if<ImageObject>(&object)) {
            h.rect(im->bounds);
            h.i32(im->sourceWidth);
            h.i32(im->sourceHeight);
            h.byte(static_cast<std::uint8_t>(im->pattern));
            h.u32(im->colorA);
            h.u32(im->colorB);
            h.u32(im->seed);
        } else if (const auto* st = std::get_if<Stroke>(&object)) {
            h.u64(static_cast<std::uint64_t>(st->points.size()));
            for (const StrokePoint& p : st->points) {
                h.f64(p.x);
                h.f64(p.y);
                h.f32(p.pressure);
            }
            h.f64(st->baseWidth);
            h.u32(st->color);
        }
    }
    return h.value();
}

} // namespace actcraft::core
