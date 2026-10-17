#include <actcraft/formats/nativefile.h>
#include <actcraft/formats/nativejson.h>
#include <actcraft/formats/urlio.h>

#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

#include <miniz.h>
#include <optional>
#include <set>
#include <string>
#include <utility>

namespace actcraft::formats {

static_assert(kMaxArchiveSize <= kMaxUrlRead,
              "une archive écrite doit se relire par readUrl et sa borne par défaut");

using namespace Qt::StringLiterals;

namespace {

std::string resourceEntry(const std::string& key, const std::string& extension)
{
    return "resources/" + key + (extension.empty() ? std::string() : "." + extension);
}

std::string thumbnailEntry(std::size_t page)
{
    return "thumbnails/page-" + std::to_string(page + 1) + ".png";
}

// Noms de ce format (casse ASCII ignorée, comme l'index du lecteur) : document.json, resources/, thumbnails/.
// Ce que cette version y trouve sans le lire (ressource non déclarée, miniature en trop) n'est pas gardé.
bool isFormatEntry(const QByteArray& name)
{
    const QByteArray lower = name.toLower();
    return lower == "document.json" || lower.startsWith("resources/") || lower.startsWith("thumbnails/");
}

// Entrée inconnue qu'on peut garder et réécrire (L2-C8) : nom UTF-8 valide d'au plus 1024 octets, relatif,
// segments non vides, ni « . » ni « .. », ni barre inverse, ni deux-points, ni caractère de contrôle. Un nom
// qui viserait hors du dossier d'extraction d'un autre outil n'est jamais réécrit.
bool isSafeEntryName(const QByteArray& name)
{
    if (name.isEmpty() || name.size() > 1024 || QString::fromUtf8(name).toUtf8() != name) {
        return false;
    }
    for (const char c : name) {
        const auto u = static_cast<unsigned char>(c);
        if (u < 0x20 || u == 0x7F || c == '\\' || c == ':') {
            return false;
        }
    }
    for (const QByteArray& segment : name.split('/')) {
        if (segment.isEmpty() || segment == "." || segment == "..") {
            return false;
        }
    }
    return true;
}

enum class Extracted
{
    Ok,
    Absent,
    TooBig,  // taille annoncée au-delà de la borne par entrée ou du budget restant
    Invalid, // entrée que miniz ne décompresse pas (dossier, tailles incohérentes, flux corrompu, CRC)
};

QString describe(Extracted status)
{
    return status == Extracted::TooBig ? u"trop volumineuse"_s : u"illisible (entrée corrompue)"_s;
}

// Lecteur zip en mémoire, fermé à la destruction. Les noms sont indexés une fois, à l'identique (octets
// exacts) : pas de mz_zip_reader_locate_file, dont la recherche par défaut ignore la casse et, devant deux
// entrées de même nom, en rend une sans dire laquelle. Une archive dont deux noms ne diffèrent que par la
// casse ASCII est refusée : un outil qui l'ignore ne saurait lequel lire.
class ZipReader
{
public:
    ZipReader(const QByteArray& archive, const ArchiveLimits& limits) : m_limits(limits)
    {
        if (archive.size() > limits.maxArchiveSize) {
            m_error = u"archive .actcraft trop volumineuse (%1 octets, au plus %2)"_s.arg(archive.size())
                          .arg(limits.maxArchiveSize);
            return;
        }
        // Pas de tri du répertoire central : l'index ci-dessous remplace la recherche de miniz.
        if (archive.isEmpty() ||
            !mz_zip_reader_init_mem(&m_zip, archive.constData(), static_cast<size_t>(archive.size()),
                                    MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY)) {
            m_error = u"archive .actcraft illisible"_s;
            return;
        }
        m_initialized = true;
        const mz_uint count = mz_zip_reader_get_num_files(&m_zip);
        if (count > limits.maxEntryCount) {
            m_error = u"archive .actcraft : %1 entrées, au plus %2"_s.arg(count).arg(limits.maxEntryCount);
            return;
        }
        QSet<QByteArray> folded;
        folded.reserve(static_cast<qsizetype>(count));
        m_index.reserve(static_cast<qsizetype>(count));
        for (mz_uint i = 0; i < count; ++i) {
            const mz_uint size = mz_zip_reader_get_filename(&m_zip, i, nullptr, 0); // longueur + 1
            if (size == 0) {
                m_error = u"archive .actcraft illisible"_s;
                return;
            }
            QByteArray name(static_cast<qsizetype>(size), Qt::Uninitialized);
            mz_zip_reader_get_filename(&m_zip, i, name.data(), size);
            name.chop(1);                          // zéro final
            const QByteArray key = name.toLower(); // casse ASCII seulement
            if (folded.contains(key)) {
                m_error = u"archive .actcraft ambiguë : entrée « %1 » en double"_s.arg(
                    QString::fromUtf8(name.left(200)));
                return;
            }
            folded.insert(key);
            m_index.insert(name, i);
            m_names << QString::fromUtf8(name);
            m_rawNames << name;
        }
        m_open = true;
    }
    ~ZipReader()
    {
        if (m_initialized) {
            mz_zip_reader_end(&m_zip);
        }
    }
    ZipReader(const ZipReader&) = delete;
    ZipReader& operator=(const ZipReader&) = delete;

    bool isOpen() const noexcept { return m_open; }
    const QString& error() const noexcept { return m_error; }
    const QStringList& names() const noexcept { return m_names; }
    const QList<QByteArray>& rawNames() const noexcept { return m_rawNames; } // octets exacts, même ordre

    Extracted extract(const std::string& name, QByteArray& out)
    {
        const auto it = m_index.constFind(QByteArray::fromStdString(name));
        if (it == m_index.cend()) {
            return Extracted::Absent;
        }
        const mz_uint index = it.value();
        mz_zip_archive_file_stat stat{};
        if (!mz_zip_reader_file_stat(&m_zip, index, &stat)) {
            return Extracted::Invalid;
        }
        // miniz rend « réussi » sans rien écrire pour un dossier (attribut DOS) et pour une entrée sans octet
        // compressé (cohérence vérifiée par miniz hors zip64 seulement) : le tampon resterait non initialisé.
        if (stat.m_is_directory || (stat.m_uncomp_size != 0 && stat.m_comp_size == 0)) {
            return Extracted::Invalid;
        }
        // Taille annoncée bornée avant d'allouer ; le tampon a cette taille et miniz n'écrit jamais au-delà
        // (une entrée qui décompresse plus que l'annonce échoue).
        if (stat.m_uncomp_size > static_cast<mz_uint64>(m_limits.maxEntry) ||
            m_total + static_cast<qint64>(stat.m_uncomp_size) > m_limits.maxTotal) {
            return Extracted::TooBig;
        }
        // Imputé AVANT d'extraire : miniz ne vérifie le CRC qu'après avoir tout décompressé, une extraction
        // ratée coûte autant qu'une réussie (et des en-têtes centraux peuvent tous désigner le même flux).
        m_total += static_cast<qint64>(stat.m_uncomp_size);
        QByteArray data(static_cast<qsizetype>(stat.m_uncomp_size), Qt::Uninitialized);
        if (!mz_zip_reader_extract_to_mem(&m_zip, index, data.data(), static_cast<size_t>(data.size()), 0)) {
            return Extracted::Invalid;
        }
        out = std::move(data);
        return Extracted::Ok;
    }

private:
    mz_zip_archive m_zip{};
    ArchiveLimits m_limits;
    bool m_initialized = false; // mz_zip_reader_init_mem réussi : à fermer
    bool m_open = false;        // et noms indexés : lisible
    QString m_error;
    QHash<QByteArray, mz_uint> m_index;
    QStringList m_names; // ordre de l'archive
    QList<QByteArray> m_rawNames;
    qint64 m_total = 0; // octets décompressés ou tentés jusqu'ici
};

} // namespace

QByteArray encodeActcraft(const core::Document& document, const std::vector<QByteArray>& thumbnailsPng,
                          QString* error, const ArchiveLimits& limits)
{
    const auto refuse = [error](const QString& why) {
        if (error) {
            *error = why;
        }
        return QByteArray();
    };
    // Entrées dans l'ordre de l'archive : document.json (compressé), ressources, miniatures (stockées).
    const QByteArray json = QJsonDocument(documentToJson(document)).toJson(QJsonDocument::Indented);
    std::vector<std::pair<std::string, const QByteArray*>> entries;
    entries.emplace_back("document.json", &json);
    for (const auto& [key, resource] : document.resources()) {
        entries.emplace_back(resourceEntry(key, resource.extension), &resource.data);
    }
    for (std::size_t i = 0; i < thumbnailsPng.size(); ++i) {
        if (!thumbnailsPng[i].isEmpty()) {
            entries.emplace_back(thumbnailEntry(i), &thumbnailsPng[i]);
        }
    }
    // Entrées inconnues gardées à la lecture (L2-C8), octets d'origine, mêmes bornes que les ressources. La
    // lecture ne produit jamais un nom dangereux, un nom de ce format (il doublerait document.json, une
    // ressource ou une miniature) ni deux noms que la casse seule distingue (la lecture refuse l'archive) :
    // ils viennent d'un appelant, et sont refusés plutôt que perdus ou écrits en double.
    QSet<QByteArray> foldedNames;
    for (const auto& [name, entry] : document.unknownEntries()) {
        const QByteArray raw = QByteArray::fromStdString(name);
        const QString shown = QString::fromUtf8(raw.left(200));
        if (!isSafeEntryName(raw)) {
            return refuse(u"archive : entrée « %1 » : nom refusé"_s.arg(shown));
        }
        if (isFormatEntry(raw)) {
            return refuse(u"archive : entrée « %1 » : nom réservé au format"_s.arg(shown));
        }
        if (foldedNames.contains(raw.toLower())) {
            return refuse(u"archive : entrée « %1 » : nom en double (casse ignorée)"_s.arg(shown));
        }
        foldedNames.insert(raw.toLower());
        entries.emplace_back(name, &entry.data);
    }
    // Ce que la lecture refuserait sous les mêmes bornes est refusé ici : un fichier enregistré se rouvre
    // toujours.
    if (entries.size() > limits.maxEntryCount) {
        return refuse(u"archive : %1 entrées, au plus %2"_s.arg(entries.size()).arg(limits.maxEntryCount));
    }
    qint64 total = 0;
    for (const auto& [name, data] : entries) {
        if (data->size() > limits.maxEntry) {
            return refuse(u"archive : entrée « %1 » trop volumineuse (%2 octets, au plus %3)"_s
                              .arg(QString::fromStdString(name))
                              .arg(data->size())
                              .arg(limits.maxEntry));
        }
        total += data->size();
    }
    if (total > limits.maxTotal) {
        return refuse(u"archive : %1 octets au total, au plus %2"_s.arg(total).arg(limits.maxTotal));
    }

    mz_zip_archive zip{};
    if (!mz_zip_writer_init_heap(&zip, 0, 64 * 1024)) {
        return refuse(u"archive : initialisation impossible"_s);
    }
    // Horodatage des entrées : nul (miniz compilé avec MINIZ_NO_TIME), d'où la reproductibilité. Les entrées
    // inconnues sont compressées (la lecture en accepte maxTotal décompressés, l'archive est bornée à
    // maxArchiveSize) ; miniz range telle quelle toute entrée que la compression grossirait.
    const std::size_t knownCount = entries.size() - document.unknownEntries().size();
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& [name, data] = entries[i];
        const auto level = static_cast<mz_uint>(i == 0            ? MZ_BEST_COMPRESSION
                                                : i >= knownCount ? MZ_DEFAULT_LEVEL
                                                                  : MZ_NO_COMPRESSION);
        if (!mz_zip_writer_add_mem(&zip, name.c_str(), data->constData(), static_cast<size_t>(data->size()),
                                   level)) {
            mz_zip_writer_end(&zip);
            return refuse(u"archive : entrée « %1 » non écrite"_s.arg(QString::fromStdString(name)));
        }
    }
    void* buffer = nullptr;
    size_t size = 0;
    if (!mz_zip_writer_finalize_heap_archive(&zip, &buffer, &size)) {
        mz_zip_writer_end(&zip);
        return refuse(u"archive : finalisation impossible"_s);
    }
    QByteArray out(static_cast<const char*>(buffer), static_cast<qsizetype>(size));
    mz_free(buffer);
    mz_zip_writer_end(&zip);
    if (out.size() > limits.maxArchiveSize) {
        return refuse(
            u"archive trop volumineuse (%1 octets, au plus %2)"_s.arg(out.size()).arg(limits.maxArchiveSize));
    }
    return out;
}

DecodeResult decodeActcraft(const QByteArray& archive, qint64 maxTotal)
{
    ArchiveLimits limits;
    limits.maxTotal = maxTotal;
    return decodeActcraft(archive, limits);
}

DecodeResult decodeActcraft(const QByteArray& archive, const ArchiveLimits& limits, Thumbnails thumbnails)
{
    DecodeResult result;
    ZipReader zip(archive, limits);
    if (!zip.isOpen()) {
        result.error = zip.error();
        return result;
    }
    QByteArray json;
    switch (zip.extract("document.json", json)) {
    case Extracted::Ok:
        break;
    case Extracted::Absent:
        result.error = u"document.json absent de l'archive"_s;
        return result;
    case Extracted::TooBig:
        result.error = u"document.json trop volumineux"_s;
        return result;
    case Extracted::Invalid:
        result.error = u"document.json illisible : entrée corrompue"_s;
        return result;
    }
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        result.error = u"document.json illisible : %1"_s.arg(parseError.errorString());
        return result;
    }
    if (!doc.isObject()) {
        result.error = u"document.json illisible : un objet JSON est attendu"_s;
        return result;
    }
    std::set<std::string> consumed{"document.json"}; // entrées du format que cette version a cherché à lire
    // La cause d'un refus est rendue typée : documentFromJson rédige l'avertissement (L2-C9).
    const JsonReadResult read = documentFromJson(
        doc.object(),
        [&zip, &consumed](const std::string& key, const std::string& extension) -> LoadedResource {
            QByteArray data;
            consumed.insert(resourceEntry(key, extension));
            switch (zip.extract(resourceEntry(key, extension), data)) {
            case Extracted::Ok:
                return data;
            case Extracted::Absent:
                return ResourceFailure::Absent;
            case Extracted::TooBig:
                return ResourceFailure::TooBig;
            case Extracted::Invalid:
                break;
            }
            return ResourceFailure::Corrupt;
        },
        result.document);
    if (!read.ok) {
        result.error = read.error;
        return result;
    }
    result.warnings = read.warnings;
    // Miniatures des pages : connues dans tous les cas ; extraites seulement si l'appelant les veut (L2-C9).
    for (std::size_t i = 0; i < result.document.pageCount(); ++i) {
        consumed.insert(thumbnailEntry(i));
    }
    if (thumbnails == Thumbnails::Read) {
        result.thumbnails.resize(result.document.pageCount());
    }
    for (std::size_t i = 0; i < result.thumbnails.size(); ++i) {
        const Extracted status = zip.extract(thumbnailEntry(i), result.thumbnails[i]);
        if (status == Extracted::TooBig || status == Extracted::Invalid) {
            result.warnings << u"miniature de la page %1 %2, ignorée"_s.arg(i + 1).arg(describe(status));
        }
    }
    // Entrées que cette version ne connaît pas (version plus récente, L2-C8) : gardées telles quelles, sous
    // le même budget de décompression que les ressources ; un dossier n'a pas d'octets, un nom dangereux est
    // écarté (jamais réécrit). Une entrée au nom du format que cette version n'a pas lue (ressource non
    // déclarée, miniature d'une page qui n'existe pas) est perdue à l'enregistrement : on le dit.
    for (const QByteArray& name : zip.rawNames()) {
        if (name.endsWith('/')) {
            continue;
        }
        const QString shown = QString::fromUtf8(name.left(200));
        if (isFormatEntry(name)) {
            if (!consumed.count(name.toStdString())) {
                result.warnings << u"entrée « %1 » : non lue par cette version, écartée"_s.arg(shown);
            }
            continue;
        }
        if (!isSafeEntryName(name)) {
            result.warnings << u"entrée « %1 » : nom refusé, écartée"_s.arg(shown);
            continue;
        }
        QByteArray data;
        const Extracted status = zip.extract(name.toStdString(), data);
        if (status != Extracted::Ok) {
            result.warnings << u"entrée « %1 » %2, écartée"_s.arg(shown, describe(status));
            continue;
        }
        result.document.setUnknownEntry(name.toStdString(), std::move(data));
    }
    result.ok = true;
    return result;
}

QStringList archiveEntryNames(const QByteArray& archive)
{
    const ZipReader zip(archive, ArchiveLimits{});
    return zip.isOpen() ? zip.names() : QStringList();
}

bool looksLikeActcraft(const QByteArray& head) noexcept
{
    return head.startsWith("PK\x03\x04");
}

} // namespace actcraft::formats
