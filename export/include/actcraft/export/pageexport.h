#pragma once
#include <actcraft/core/model.h>

#include <QFuture>
#include <QString>
#include <QUrl>

namespace actcraft::exporting { // « export » est un mot réservé du C++

enum class ImageFormat : std::uint8_t
{
    Png,
    Jpeg,
    Bmp,
    Tiff,
};
ImageFormat imageFormatFor(const QUrl& url); // d'après l'extension ; PNG par défaut
struct ExportResult
{
    bool ok = false;
    QString error; // texte à nous, pour la bannière
};
// Copie du document prise par valeur sur le fil appelant ; rendu par render::renderPage sur un fil
// secondaire, à la taille de la page en pixels.
QFuture<ExportResult> exportPageImage(core::Document document, std::size_t page, QUrl url);

} // namespace actcraft::exporting
