#include <actcraft/render/imagecolor.h>

#include <QList>

namespace actcraft::render {

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

QImage withKeyColor(const QImage& source, core::Argb transparent)
{
    QImage image = source.convertToFormat(QImage::Format_ARGB32);
    const QRgb key = transparent & 0x00FFFFFFu;
    for (int y = 0; y < image.height(); ++y) {
        auto* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            if ((line[x] & 0x00FFFFFFu) == key) {
                line[x] = 0;
            }
        }
    }
    return image;
}

} // namespace actcraft::render
