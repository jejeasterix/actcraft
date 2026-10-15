#include <actcraft/formats/nativefile.h>
#include <actcraft/formats/nativejson.h>
#include <actcraft/formats/urlio.h>

#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

#include <map>
#include <miniz.h>
#include <optional>
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
    qint64 m_total = 0;  // octets décompressés ou tentés jusqu'ici
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
    // Horodatage des entrées : nul (miniz compilé avec MINIZ_NO_TIME), d'où la reproductibilité.
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& [name, data] = entries[i];
        const auto level = static_cast<mz_uint>(i == 0 ? MZ_BEST_COMPRESSION : MZ_NO_COMPRESSION);
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

DecodeResult decodeActcraft(const QByteArray& archive, const ArchiveLimits& limits)
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
    std::map<std::string, Extracted> refused; // ressources présentes mais refusées, et pourquoi
    const JsonReadResult read = documentFromJson(
        doc.object(),
        [&zip, &refused](const std::string& key, const std::string& extension) -> std::optional<QByteArray> {
            QByteArray data;
            const Extracted status = zip.extract(resourceEntry(key, extension), data);
            if (status != Extracted::Ok) {
                if (status != Extracted::Absent) {
                    refused[key] = status;
                }
                return std::nullopt;
            }
            return data;
        },
        result.document);
    if (!read.ok) {
        result.error = read.error;
        return result;
    }
    result.warnings = read.warnings;
    // documentFromJson dit « absente » de toute ressource que le chargeur ne rend pas : la vraie cause la
    // remplace.
    for (const auto& [key, status] : refused) {
        const QString absent = u"ressource %1 absente, écartée"_s.arg(QString::fromStdString(key));
        const QString cause =
            u"ressource %1 %2, écartée"_s.arg(QString::fromStdString(key), describe(status));
        bool replaced = false;
        for (QString& warning : result.warnings) {
            if (warning.endsWith(absent)) {
                warning.replace(warning.size() - absent.size(), absent.size(), cause);
                replaced = true;
                break;
            }
        }
        if (!replaced) {
            result.warnings << cause;
        }
    }
    result.thumbnails.resize(result.document.pageCount());
    for (std::size_t i = 0; i < result.thumbnails.size(); ++i) {
        const Extracted status = zip.extract(thumbnailEntry(i), result.thumbnails[i]);
        if (status == Extracted::TooBig || status == Extracted::Invalid) {
            result.warnings << u"miniature de la page %1 %2, ignorée"_s.arg(i + 1).arg(describe(status));
        }
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
