#include <actcraft/bench/contentcheck.h>

#include <cmath>
#include <cstdlib>

namespace actcraft::bench {

namespace {

// Parcours de Bresenham de [from, to], extrémités comprises : nombre de pixels dont la couleur vérifie
// `keep`. -1 si l'image est vide ou si un point sort de l'image.
template <typename Keep> int countAlongLine(const QImage& image, QPoint from, QPoint to, Keep keep)
{
    if (image.isNull() || !image.rect().contains(from) || !image.rect().contains(to)) {
        return -1;
    }
    int x = from.x();
    int y = from.y();
    const int dx = std::abs(to.x() - x);
    const int dy = -std::abs(to.y() - y);
    const int sx = x < to.x() ? 1 : -1;
    const int sy = y < to.y() ? 1 : -1;
    int err = dx + dy;
    int count = 0;
    for (;;) {
        if (keep(image.pixelColor(x, y))) {
            ++count;
        }
        if (x == to.x() && y == to.y()) {
            break;
        }
        const int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y += sy;
        }
    }
    return count;
}

} // namespace

bool colorNear(QColor actual, QColor expected, int tolerance) noexcept
{
    return std::abs(actual.red() - expected.red()) <= tolerance &&
           std::abs(actual.green() - expected.green()) <= tolerance &&
           std::abs(actual.blue() - expected.blue()) <= tolerance;
}

QColor sampleColor(const QImage& image, QPoint pixel)
{
    if (!image.rect().contains(pixel)) {
        return {};
    }
    int r = 0;
    int g = 0;
    int b = 0;
    int n = 0;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            const QPoint p(pixel.x() + dx, pixel.y() + dy);
            if (image.rect().contains(p)) {
                const QColor c = image.pixelColor(p);
                r += c.red();
                g += c.green();
                b += c.blue();
                ++n;
            }
        }
    }
    return QColor(r / n, g / n, b / n);
}

QPoint sceneToImage(QPointF scenePoint, qreal devicePixelRatio)
{
    // Le pixel i de la capture couvre [i, i + 1[ (centre en i + 0,5), comme pour le rastériseur : le point
    // tombe dans le pixel floor(p). Arrondir le décalait d'un demi-pixel en moyenne, d'un pixel au pire, et
    // l'échantillon 3 × 3 débordait d'un trait de 3 pixels (L2-C11 : #555555 en D3D11 au zoom 0,9).
    return QPoint(static_cast<int>(std::floor(scenePoint.x() * devicePixelRatio)),
                  static_cast<int>(std::floor(scenePoint.y() * devicePixelRatio)));
}

ContentCheck checkColor(const QString& name, const QImage& image, QPoint pixel, QColor expected,
                        int tolerance)
{
    const QColor actual = sampleColor(image, pixel);
    ContentCheck check;
    check.name = name;
    check.expected = expected.name(QColor::HexArgb);
    check.actual = actual.isValid() ? actual.name(QColor::HexArgb) : QStringLiteral("hors-image");
    check.ok = actual.isValid() && colorNear(actual, expected, tolerance);
    return check;
}

int countIntermediatePixels(const QImage& image, QPoint from, QPoint to, QColor background, QColor ink,
                            int margin)
{
    return countAlongLine(image, from, to, [&](QColor c) {
        return !colorNear(c, background, margin) && !colorNear(c, ink, margin);
    });
}

int countInkPixels(const QImage& image, QPoint from, QPoint to, QColor ink, int margin)
{
    return countAlongLine(image, from, to, [&](QColor c) { return colorNear(c, ink, margin); });
}

QStringList captureSteps()
{
    return {QStringLiteral("mi-trait"), QStringLiteral("fin-trait"), QStringLiteral("zoom-800"),
            QStringLiteral("retour-100")};
}

QString captureFileName(const QString& graphicsApi, const QString& step)
{
    return graphicsApi + QLatin1Char('-') + step + QStringLiteral(".png");
}

} // namespace actcraft::bench
