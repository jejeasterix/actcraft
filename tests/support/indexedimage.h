#pragma once

#include <QByteArray>
#include <QImage>
#include <QList>
#include <QRgb>

#include <algorithm>

// Images indexées écrites par nous pour les tests (L2-C7) : rien n'est tiré d'un paperboard réel.
namespace indexedtest {

// Image 8 bits indexée : table de `colors` rouges opaques (vide si 0), tous les pixels à l'index `index`,
// octets de bourrage de fin de ligne (au-delà de la largeur, jusqu'à bytesPerLine) à `border`.
inline QImage indexedImage(int width, int height, int colors, uchar index, uchar border)
{
    QImage image(width, height, QImage::Format_Indexed8);
    image.setColorTable(QList<QRgb>(colors, qRgb(255, 0, 0)));
    for (int y = 0; y < height; ++y) {
        uchar* line = image.scanLine(y);
        std::fill(line, line + width, index);
        std::fill(line + width, line + image.bytesPerLine(), border);
    }
    return image;
}

inline quint32 pngCrc(const QByteArray& data)
{
    quint32 crc = 0xFFFFFFFFu;
    for (const char c : data) {
        crc ^= static_cast<quint8>(c);
        for (int k = 0; k < 8; ++k) {
            crc = (crc & 1u) != 0 ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
        }
    }
    return crc ^ 0xFFFFFFFFu;
}

inline void appendBigEndian(QByteArray& out, quint32 value)
{
    for (int shift = 24; shift >= 0; shift -= 8) {
        out.append(static_cast<char>((value >> shift) & 0xFFu));
    }
}

inline void appendChunk(QByteArray& png, const char* type, const QByteArray& body)
{
    QByteArray typed(type, 4);
    typed.append(body);
    appendBigEndian(png, static_cast<quint32>(body.size()));
    png.append(typed);
    appendBigEndian(png, pngCrc(typed));
}

// PNG à palette (8 bits par pixel, filtre 0, sans entrelacement) écrit à la main depuis `image` (Indexed8) :
// sa table en palette, ses index tels quels, même hors de la palette. Le lecteur de libpng accepte un tel
// fichier (avertissement), son écrivain le refuse : QImage::save ne peut pas le produire.
inline QByteArray indexedPng(const QImage& image)
{
    QByteArray png("\x89PNG\r\n\x1a\n", 8);
    QByteArray header;
    appendBigEndian(header, static_cast<quint32>(image.width()));
    appendBigEndian(header, static_cast<quint32>(image.height()));
    header.append(static_cast<char>(8)); // bits par index
    header.append(static_cast<char>(3)); // couleurs à palette
    header.append(3, '\0');              // compression, filtre, entrelacement
    appendChunk(png, "IHDR", header);
    QByteArray palette;
    for (const QRgb color : image.colorTable()) {
        palette.append(static_cast<char>(qRed(color)));
        palette.append(static_cast<char>(qGreen(color)));
        palette.append(static_cast<char>(qBlue(color)));
    }
    appendChunk(png, "PLTE", palette);
    QByteArray rows;
    for (int y = 0; y < image.height(); ++y) {
        rows.append('\0'); // filtre 0 : ligne brute
        rows.append(reinterpret_cast<const char*>(image.constScanLine(y)), image.width());
    }
    appendChunk(png, "IDAT", qCompress(rows).mid(4)); // qCompress : 4 octets de longueur, puis un flux zlib
    appendChunk(png, "IEND", QByteArray());
    return png;
}

} // namespace indexedtest
