#include <actcraft/render/imagesynth.h>

#include <algorithm>
#include <cmath>

namespace actcraft::render {

namespace {

QRgb mix(QRgb a, QRgb b, int t) // t de 0 à 255
{
    const auto channel = [t](int ca, int cb) { return (ca * (255 - t) + cb * t) / 255; };
    return qRgb(channel(qRed(a), qRed(b)), channel(qGreen(a), qGreen(b)), channel(qBlue(a), qBlue(b)));
}

} // namespace

QImage withoutColorTable(const QImage& image)
{
    qsizetype entries = 0;
    if (image.format() == QImage::Format_Indexed8) {
        entries = 256;
    } else if (image.format() == QImage::Format_Mono || image.format() == QImage::Format_MonoLSB) {
        entries = 2;
    } else {
        return image;
    }
    QList<QRgb> table = image.colorTable();
    if (table.size() < entries) {
        table.resize(entries, 0u); // index sans couleur : pixel transparent
    }
    QImage complete = image;
    complete.setColorTable(table);
    return complete.convertToFormat(QImage::Format_ARGB32);
}

QImage synthesizeImage(const core::ImageObject& object)
{
    const int w = std::max(1, static_cast<int>(object.sourceWidth));
    const int h = std::max(1, static_cast<int>(object.sourceHeight));
    QImage image(w, h, QImage::Format_ARGB32_Premultiplied);
    const QRgb a = object.colorA | 0xFF000000u;
    const QRgb b = object.colorB | 0xFF000000u;
    const int phase = static_cast<int>(object.seed % 32u);
    for (int y = 0; y < h; ++y) {
        auto* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < w; ++x) {
            QRgb c = a;
            switch (object.pattern) {
            case core::ImagePattern::LinearGradient:
                c = mix(a, b, (x + y) * 255 / std::max(1, w + h - 2));
                break;
            case core::ImagePattern::Checker:
                c = (((x + phase) / 32 + (y + phase) / 32) % 2 == 0) ? a : b;
                break;
            case core::ImagePattern::Rings: {
                const int dx = x - w / 2;
                const int dy = y - h / 2;
                const int ring = static_cast<int>(std::sqrt(static_cast<double>(dx * dx + dy * dy))) / 24;
                c = ring % 2 == 0 ? a : b;
                break;
            }
            case core::ImagePattern::SolidCenter:
                c = (x >= w / 4 && x < w - w / 4 && y >= h / 4 && y < h - h / 4) ? a : b;
                break;
            }
            line[x] = c;
        }
    }
    return image;
}

} // namespace actcraft::render
