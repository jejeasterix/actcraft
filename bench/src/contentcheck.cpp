#include <actcraft/bench/contentcheck.h>

#include <cmath>

namespace actcraft::bench {

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
    return QPoint(static_cast<int>(std::lround(scenePoint.x() * devicePixelRatio)),
                  static_cast<int>(std::lround(scenePoint.y() * devicePixelRatio)));
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
