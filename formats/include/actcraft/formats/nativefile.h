#pragma once

#include <actcraft/core/document.h>

#include <QByteArray>
#include <QString>
#include <QStringList>

#include <cstddef>
#include <vector>

namespace actcraft::formats {

// Une entrée décompressée de plus de 512 Mio est refusée avant toute allocation (archive non fiable), et une
// archive dont les entrées lues dépasseraient 2 Gio au total aussi (budget de décompression, imputé avant
// chaque extraction, réussie ou non).
inline constexpr qint64 kMaxArchiveEntry = 512ll * 1024 * 1024;
inline constexpr qint64 kMaxArchiveTotal = 2048ll * 1024 * 1024;
// Nombre d'entrées d'une archive (limite du zip classique) : au-delà, refusée avant d'indexer les noms.
inline constexpr std::size_t kMaxArchiveEntryCount = 65535;
// Taille de l'archive elle-même : celle que readUrl lit par défaut (kMaxUrlRead, vérifié par nativefile.cpp).
inline constexpr qint64 kMaxArchiveSize = 1024ll * 1024 * 1024;

// Bornes de la lecture. L'écriture refuse tout ce que la lecture refuserait sous les mêmes bornes : un
// fichier enregistré se rouvre toujours. Réduites dans les tests, pour les prouver sans allouer des Gio.
struct ArchiveLimits
{
    qint64 maxEntry = kMaxArchiveEntry;
    qint64 maxTotal = kMaxArchiveTotal;
    std::size_t maxEntryCount = kMaxArchiveEntryCount;
    qint64 maxArchiveSize = kMaxArchiveSize;
};

// Archive .actcraft (DA3) : « document.json » (compressé), « resources/<sha256>.<ext> » (stockés tels quels),
// « thumbnails/page-<n>.png » (n à partir de 1 ; octets fournis par l'appelant, rangés tels quels).
// Reproductible : mêmes entrées, mêmes octets. Vide en cas d'échec (`error` dit pourquoi).
QByteArray encodeActcraft(const core::Document& document, const std::vector<QByteArray>& thumbnailsPng,
                          QString* error = nullptr, const ArchiveLimits& limits = {});

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
DecodeResult decodeActcraft(const QByteArray& archive, const ArchiveLimits& limits);
// Noms des entrées dans l'ordre de l'archive ; vide si illisible.
QStringList archiveEntryNames(const QByteArray& archive);
// Signature zip (« PK\3\4 ») : de quoi choisir le lecteur sans lire tout le fichier.
bool looksLikeActcraft(const QByteArray& head) noexcept;

} // namespace actcraft::formats
