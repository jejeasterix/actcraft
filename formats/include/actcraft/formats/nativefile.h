#pragma once

#include <actcraft/core/document.h>

#include <QByteArray>
#include <QString>
#include <QStringList>

#include <cstddef>
#include <vector>

namespace actcraft::formats {

// Une entrée décompressée de plus de 512 Mio est refusée avant toute allocation (archive non fiable), et une
// archive dont les entrées lues dépasseraient 2 Gio au total aussi (budget de décompression).
inline constexpr qint64 kMaxArchiveEntry = 512ll * 1024 * 1024;
inline constexpr qint64 kMaxArchiveTotal = 2048ll * 1024 * 1024;
// Nombre d'entrées d'une archive (limite du zip classique) : au-delà, refusée avant d'indexer les noms ;
// l'écriture refuse aussi un document qui la dépasserait (document.json + ressources + miniatures).
inline constexpr std::size_t kMaxArchiveEntryCount = 65535;

// Archive .actcraft (DA3) : « document.json » (compressé), « resources/<sha256>.<ext> » (stockés tels quels),
// « thumbnails/page-<n>.png » (n à partir de 1 ; octets fournis par l'appelant, rangés tels quels).
// Reproductible : mêmes entrées, mêmes octets. Vide en cas d'échec (`error` dit pourquoi).
QByteArray encodeActcraft(const core::Document& document, const std::vector<QByteArray>& thumbnailsPng,
                          QString* error = nullptr);

struct DecodeResult
{
    bool ok = false;
    QString error;
    QStringList warnings;
    core::Document document;
    std::vector<QByteArray> thumbnails; // une case par page, vide si absente
};

// `maxTotal` : budget de décompression (petit dans les tests, pour le prouver sans archive de 2 Gio).
DecodeResult decodeActcraft(const QByteArray& archive, qint64 maxTotal = kMaxArchiveTotal);
// Noms des entrées dans l'ordre de l'archive ; vide si illisible.
QStringList archiveEntryNames(const QByteArray& archive);
// Signature zip (« PK\3\4 ») : de quoi choisir le lecteur sans lire tout le fichier.
bool looksLikeActcraft(const QByteArray& head) noexcept;

} // namespace actcraft::formats
