#include <actcraft/core/generator.h>

#include <array>

namespace actcraft::core {

std::uint32_t PortableRng::next()
{
    return static_cast<std::uint32_t>(m_engine() & 0xFFFFFFFFu);
}

std::uint32_t PortableRng::below(std::uint32_t n)
{
    const std::uint64_t range = 1ull << 32;
    const std::uint64_t limit = range - range % n;
    std::uint64_t x = 0;
    do {
        x = next();
    } while (x >= limit);
    return static_cast<std::uint32_t>(x % n);
}

std::int32_t PortableRng::between(std::int32_t lo, std::int32_t hi)
{
    return lo + static_cast<std::int32_t>(below(static_cast<std::uint32_t>(hi - lo + 1)));
}

namespace {

// Coordonnées internes en 1/16 de pixel (décision 3).
constexpr std::int32_t kSub = 16;
constexpr std::int32_t kPageW = 1024 * kSub;
constexpr std::int32_t kPageH = 768 * kSub;
constexpr std::int32_t kEdge = 8 * kSub;           // les traits restent à 8 px du bord
constexpr std::int32_t kWitnessMargin = 16 * kSub; // et à 16 px des témoins

struct IntRect
{
    std::int32_t x;
    std::int32_t y;
    std::int32_t w;
    std::int32_t h;
};

// Témoins, en pixels de page entiers.
constexpr IntRect kShapeWitness{16, 16, 64, 64};
constexpr IntRect kImageWitness{944, 16, 64, 64};
constexpr IntRect kStrokeWitness{16, 708, 64, 24};
constexpr std::array<IntRect, 3> kWitnessRects{kShapeWitness, kImageWitness, kStrokeWitness};

// 16 directions, vecteur unitaire × 16, arrondi (aucun sin/cos à l'exécution).
constexpr std::array<std::array<std::int32_t, 2>, 16> kDirections{{
    {16, 0},
    {15, 6},
    {11, 11},
    {6, 15},
    {0, 16},
    {-6, 15},
    {-11, 11},
    {-15, 6},
    {-16, 0},
    {-15, -6},
    {-11, -11},
    {-6, -15},
    {0, -16},
    {6, -15},
    {11, -11},
    {15, -6},
}};

double toPage(std::int32_t sub)
{
    return static_cast<double>(sub) / kSub; // division exacte : puissance de deux
}

bool hitsWitnessSub(std::int32_t x, std::int32_t y)
{
    for (const IntRect& r : kWitnessRects) {
        const std::int32_t left = r.x * kSub - kWitnessMargin;
        const std::int32_t top = r.y * kSub - kWitnessMargin;
        const std::int32_t right = (r.x + r.w) * kSub + kWitnessMargin;
        const std::int32_t bottom = (r.y + r.h) * kSub + kWitnessMargin;
        if (x >= left && x <= right && y >= top && y <= bottom) {
            return true;
        }
    }
    return false;
}

// Couleur opaque, canaux de 0x40 à 0xEF : jamais proche du noir de l'encre (contrôle de contenu).
Argb randomColor(PortableRng& rng)
{
    const auto r = static_cast<Argb>(rng.between(0x40, 0xEF));
    const auto g = static_cast<Argb>(rng.between(0x40, 0xEF));
    const auto b = static_cast<Argb>(rng.between(0x40, 0xEF));
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

Shape makeShape(PortableRng& rng, int index)
{
    Shape shape;
    shape.kind = static_cast<ShapeKind>(rng.below(3));
    const std::int32_t w = rng.between(16, 160) * kSub;
    const std::int32_t h = rng.between(16, 160) * kSub;
    const std::int32_t x = rng.between(0, kPageW - w);
    const std::int32_t y = rng.between(0, kPageH - h);
    shape.bounds = {toPage(x), toPage(y), toPage(w), toPage(h)};
    shape.fill = randomColor(rng);
    shape.outline = randomColor(rng);
    shape.outlineWidth = static_cast<double>(rng.between(1, 4));
    shape.opacity = index % 5 == 0 ? 0.5 : 1.0; // 30 formes sur 150 (N1-5)
    if (shape.kind == ShapeKind::Polygon) {
        const std::int32_t k = rng.between(3, 8);
        const std::int32_t cx = x + w / 2;
        const std::int32_t cy = y + h / 2;
        for (std::int32_t j = 0; j < k; ++j) {
            const auto dir = kDirections[static_cast<std::size_t>((j * 8 / k) * 2)];
            const std::int32_t vx = cx + dir[0] * (w / 2) / 16;
            const std::int32_t vy = cy + dir[1] * (h / 2) / 16;
            shape.polygon.push_back({toPage(vx), toPage(vy)});
        }
    }
    return shape;
}

ImageObject makeImage(PortableRng& rng)
{
    ImageObject image;
    image.sourceWidth = rng.between(16, 64) * 16;
    image.sourceHeight = rng.between(16, 64) * 16;
    const std::int32_t dw = rng.between(48, 240);
    std::int32_t dh = dw * image.sourceHeight / image.sourceWidth;
    // Proportions gardées, hauteur bornée : une image haute (256 × 1024) donnerait jusqu'à 960 px > 768,
    // et between(0, (768 − dh) × 16) recevrait une borne négative.
    if (dh < 16) {
        dh = 16;
    } else if (dh > 768 - 16) {
        dh = 768 - 16;
    }
    const std::int32_t x = rng.between(0, (1024 - dw) * kSub);
    const std::int32_t y = rng.between(0, (768 - dh) * kSub);
    image.bounds = {toPage(x), toPage(y), static_cast<double>(dw), static_cast<double>(dh)};
    image.pattern = static_cast<ImagePattern>(rng.below(3));
    image.colorA = randomColor(rng);
    image.colorB = randomColor(rng);
    image.seed = rng.next();
    return image;
}

bool validStrokePoint(std::int32_t x, std::int32_t y)
{
    return x >= kEdge && x <= kPageW - kEdge && y >= kEdge && y <= kPageH - kEdge && !hitsWitnessSub(x, y);
}

Stroke makeStroke(PortableRng& rng)
{
    Stroke stroke;
    const std::int32_t count = rng.between(50, 300);
    stroke.baseWidth = static_cast<double>(rng.between(2, 6));
    stroke.color = randomColor(rng);
    std::int32_t x = 0;
    std::int32_t y = 0;
    do {
        x = rng.between(kEdge, kPageW - kEdge);
        y = rng.between(kEdge, kPageH - kEdge);
    } while (!validStrokePoint(x, y));
    std::int32_t dir = static_cast<std::int32_t>(rng.below(16));
    std::int32_t pressure = rng.between(256, 1023); // en 1/1024
    stroke.points.reserve(static_cast<std::size_t>(count));
    for (std::int32_t i = 0; i < count; ++i) {
        stroke.points.push_back({toPage(x), toPage(y), static_cast<float>(pressure) / 1024.0f});
        dir = (dir + 16 + rng.between(-1, 1)) % 16;
        const std::int32_t length = rng.between(3, 8);
        for (std::int32_t attempt = 0; attempt < 16; ++attempt) {
            const auto d = kDirections[static_cast<std::size_t>((dir + attempt) % 16)];
            const std::int32_t nx = x + d[0] * length;
            const std::int32_t ny = y + d[1] * length;
            if (validStrokePoint(nx, ny)) {
                x = nx;
                y = ny;
                dir = (dir + attempt) % 16;
                break;
            }
        }
        pressure += rng.between(-32, 32);
        if (pressure < 256) {
            pressure = 256;
        } else if (pressure > 1023) {
            pressure = 1023;
        }
    }
    return stroke;
}

RectF toRect(const IntRect& r)
{
    return {static_cast<double>(r.x), static_cast<double>(r.y), static_cast<double>(r.w),
            static_cast<double>(r.h)};
}

void appendWitnesses(Page& page)
{
    const Witnesses& w = witnesses();
    Shape shape;
    shape.kind = ShapeKind::Rectangle;
    shape.bounds = w.shapeRect;
    shape.fill = w.shapeColor;
    shape.outline = w.shapeColor;
    shape.outlineWidth = 0;
    shape.opacity = 1;
    page.objects.emplace_back(shape);

    ImageObject image;
    image.bounds = w.imageRect;
    image.pattern = ImagePattern::SolidCenter;
    image.colorA = w.imageCenterColor;
    image.colorB = 0xFF7E57C2u;
    page.objects.emplace_back(image);

    Stroke stroke;
    stroke.baseWidth = 12;
    stroke.color = w.strokeColor;
    for (int i = 0; i < 5; ++i) {
        stroke.points.push_back({24.0 + 12.0 * i, 720.0, 0.5f});
    }
    page.objects.emplace_back(stroke);
}

} // namespace

const Witnesses& witnesses() noexcept
{
    static const Witnesses value{
        toRect(kShapeWitness),  0xFF1E88E5u,   toRect(kImageWitness), 0xFF43A047u,
        toRect(kStrokeWitness), {48.0, 720.0}, 0xFFFB8C00u,
    };
    return value;
}

bool hitsWitness(PointF p, double margin) noexcept
{
    for (const IntRect& r : kWitnessRects) {
        if (p.x >= r.x - margin && p.x <= r.x + r.w + margin && p.y >= r.y - margin &&
            p.y <= r.y + r.h + margin) {
            return true;
        }
    }
    return false;
}

Page generatePage(std::uint32_t seed)
{
    PortableRng rng(seed);
    Page page;
    page.objects.reserve(300);
    int shapes = kShapeCount - 1;
    int images = kImageCount - 1;
    int strokes = kStrokeCount - 1;
    int shapeIndex = 0;
    while (shapes + images + strokes > 0) {
        const auto pick = static_cast<int>(rng.below(static_cast<std::uint32_t>(shapes + images + strokes)));
        if (pick < shapes) {
            page.objects.emplace_back(makeShape(rng, shapeIndex++));
            --shapes;
        } else if (pick < shapes + images) {
            page.objects.emplace_back(makeImage(rng));
            --images;
        } else {
            page.objects.emplace_back(makeStroke(rng));
            --strokes;
        }
    }
    appendWitnesses(page);
    return page;
}

} // namespace actcraft::core
