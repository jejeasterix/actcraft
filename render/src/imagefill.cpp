#include <actcraft/render/imagefill.h>

#include <QRgb>

#include <cstdlib>
#include <vector>

namespace actcraft::render {

QImage floodFill(const QImage& image, QPoint seed, core::Argb color, int tolerance)
{
    if (image.isNull() || seed.x() < 0 || seed.y() < 0 || seed.x() >= image.width() || seed.y() >= image.height())
        return image;
    QImage out = image.convertToFormat(QImage::Format_ARGB32);
    const int width = out.width();
    const int height = out.height();
    const QRgb target = out.pixel(seed);
    // Écart par canal ≤ tolérance × 255 / 100, sans arrondi : écart × 100 ≤ tolérance × 255.
    const int limit = tolerance * 255;
    const auto matches = [&](QRgb pixel) {
        return std::abs(qRed(pixel) - qRed(target)) * 100 <= limit &&
               std::abs(qGreen(pixel) - qGreen(target)) * 100 <= limit &&
               std::abs(qBlue(pixel) - qBlue(target)) * 100 <= limit;
    };
    std::vector<bool> seen(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), false);
    std::vector<QPoint> queue{seed};
    const auto index = [&](QPoint p) {
        return static_cast<std::size_t>(p.y()) * static_cast<std::size_t>(width) + static_cast<std::size_t>(p.x());
    };
    seen[index(seed)] = true;
    // La zone se décide sur l'image d'origine : un pixel peint ne devient pas la référence.
    const QImage source = out;
    while (!queue.empty()) {
        const QPoint p = queue.back();
        queue.pop_back();
        const QRgb old = source.pixel(p);
        out.setPixel(p, qRgba(qRed(color), qGreen(color), qBlue(color), qAlpha(old)));
        const QPoint around[4] = {{p.x() + 1, p.y()}, {p.x() - 1, p.y()}, {p.x(), p.y() + 1}, {p.x(), p.y() - 1}};
        for (const QPoint& n : around) {
            if (n.x() < 0 || n.y() < 0 || n.x() >= width || n.y() >= height || seen[index(n)])
                continue;
            if (!matches(source.pixel(n)))
                continue;
            seen[index(n)] = true;
            queue.push_back(n);
        }
    }
    return out;
}

} // namespace actcraft::render
