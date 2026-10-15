#include <actcraft/formats/nativefile.h>
#include <actcraft/formats/nativejson.h>

#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>

#include <miniz.h>
#include <optional>
#include <string>

namespace actcraft::formats {

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

// Lecteur zip en mémoire, fermé à la destruction. Les noms sont indexés une fois, à l'identique (octets
// exacts) : pas de mz_zip_reader_locate_file, dont la recherche par défaut ignore la casse et, devant deux
// entrées de même nom, en rend une sans dire laquelle. Une archive aux noms en double est refusée.
class ZipReader
{
public:
    ZipReader(const QByteArray& archive, qint64 maxTotal) : m_maxTotal(maxTotal)
    {
        // Pas de tri du répertoire central : l'index ci-dessous remplace la recherche de miniz.
        if (archive.isEmpty() ||
            !mz_zip_reader_init_mem(&m_zip, archive.constData(), static_cast<size_t>(archive.size()),
                                    MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY)) {
            m_error = u"archive .actcraft illisible"_s;
            return;
        }
        m_initialized = true;
        const mz_uint count = mz_zip_reader_get_num_files(&m_zip);
        if (count > kMaxArchiveEntryCount) {
            m_error = u"archive .actcraft : %1 entrées, au plus %2"_s.arg(count).arg(kMaxArchiveEntryCount);
            return;
        }
        m_index.reserve(static_cast<qsizetype>(count));
        for (mz_uint i = 0; i < count; ++i) {
            const mz_uint size = mz_zip_reader_get_filename(&m_zip, i, nullptr, 0); // longueur + 1
            if (size == 0) {
                m_error = u"archive .actcraft illisible"_s;
                return;
            }
            QByteArray name(static_cast<qsizetype>(size), Qt::Uninitialized);
            mz_zip_reader_get_filename(&m_zip, i, name.data(), size);
            name.chop(1); // zéro final
            if (m_index.contains(name)) {
                m_error = u"archive .actcraft ambiguë : entrée « %1 » en double"_s.arg(
                    QString::fromUtf8(name.left(200)));
                return;
            }
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
        if (stat.m_uncomp_size > static_cast<mz_uint64>(kMaxArchiveEntry) ||
            m_total + static_cast<qint64>(stat.m_uncomp_size) > m_maxTotal) {
            return Extracted::TooBig;
        }
        QByteArray data(static_cast<qsizetype>(stat.m_uncomp_size), Qt::Uninitialized);
        if (!mz_zip_reader_extract_to_mem(&m_zip, index, data.data(), static_cast<size_t>(data.size()), 0)) {
            return Extracted::Invalid;
        }
        m_total += data.size();
        out = std::move(data);
        return Extracted::Ok;
    }

private:
    mz_zip_archive m_zip{};
    bool m_initialized = false; // mz_zip_reader_init_mem réussi : à fermer
    bool m_open = false;        // et noms indexés : lisible
    QString m_error;
    QHash<QByteArray, mz_uint> m_index;
    QStringList m_names; // ordre de l'archive
    qint64 m_total = 0;  // octets décompressés jusqu'ici
    qint64 m_maxTotal;   // budget (kMaxArchiveTotal hors des tests)
};

} // namespace

QByteArray encodeActcraft(const core::Document& document, const std::vector<QByteArray>& thumbnailsPng,
                          QString* error)
{
    std::size_t entries = 1 + document.resources().size();
    for (const QByteArray& png : thumbnailsPng) {
        if (!png.isEmpty()) {
            ++entries;
        }
    }
    if (entries > kMaxArchiveEntryCount) {
        if (error) {
            *error = u"archive : %1 entrées, au plus %2"_s.arg(entries).arg(kMaxArchiveEntryCount);
        }
        return {};
    }
    mz_zip_archive zip{};
    if (!mz_zip_writer_init_heap(&zip, 0, 64 * 1024)) {
        if (error) {
            *error = u"archive : initialisation impossible"_s;
        }
        return {};
    }
    const auto fail = [&zip, error](const QString& why) {
        mz_zip_writer_end(&zip);
        if (error) {
            *error = why;
        }
        return QByteArray();
    };
    // Horodatage des entrées : nul (miniz compilé avec MINIZ_NO_TIME), d'où la reproductibilité.
    const QByteArray json = QJsonDocument(documentToJson(document)).toJson(QJsonDocument::Indented);
    if (!mz_zip_writer_add_mem(&zip, "document.json", json.constData(), static_cast<size_t>(json.size()),
                               static_cast<mz_uint>(MZ_BEST_COMPRESSION))) {
        return fail(u"archive : document.json non écrit"_s);
    }
    for (const auto& [key, resource] : document.resources()) {
        const std::string name = resourceEntry(key, resource.extension);
        if (!mz_zip_writer_add_mem(&zip, name.c_str(), resource.data.constData(),
                                   static_cast<size_t>(resource.data.size()),
                                   static_cast<mz_uint>(MZ_NO_COMPRESSION))) {
            return fail(u"archive : ressource %1 non écrite"_s.arg(QString::fromStdString(key)));
        }
    }
    for (std::size_t i = 0; i < thumbnailsPng.size(); ++i) {
        if (thumbnailsPng[i].isEmpty()) {
            continue;
        }
        const std::string name = thumbnailEntry(i);
        if (!mz_zip_writer_add_mem(&zip, name.c_str(), thumbnailsPng[i].constData(),
                                   static_cast<size_t>(thumbnailsPng[i].size()),
                                   static_cast<mz_uint>(MZ_NO_COMPRESSION))) {
            return fail(u"archive : miniature %1 non écrite"_s.arg(i + 1));
        }
    }
    void* buffer = nullptr;
    size_t size = 0;
    if (!mz_zip_writer_finalize_heap_archive(&zip, &buffer, &size)) {
        return fail(u"archive : finalisation impossible"_s);
    }
    QByteArray out(static_cast<const char*>(buffer), static_cast<qsizetype>(size));
    mz_free(buffer);
    mz_zip_writer_end(&zip);
    return out;
}

DecodeResult decodeActcraft(const QByteArray& archive, qint64 maxTotal)
{
    DecodeResult result;
    ZipReader zip(archive, maxTotal);
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
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        result.error = u"document.json illisible : %1"_s.arg(parseError.errorString());
        return result;
    }
    const JsonReadResult read = documentFromJson(
        doc.object(),
        [&zip](const std::string& key, const std::string& extension) -> std::optional<QByteArray> {
            QByteArray data;
            if (zip.extract(resourceEntry(key, extension), data) != Extracted::Ok) {
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
    result.thumbnails.resize(result.document.pageCount());
    for (std::size_t i = 0; i < result.thumbnails.size(); ++i) {
        const Extracted status = zip.extract(thumbnailEntry(i), result.thumbnails[i]);
        if (status == Extracted::TooBig || status == Extracted::Invalid) {
            result.warnings << u"miniature de la page %1 %2, ignorée"_s.arg(i + 1).arg(
                status == Extracted::TooBig ? u"trop volumineuse"_s : u"illisible"_s);
        }
    }
    result.ok = true;
    return result;
}

QStringList archiveEntryNames(const QByteArray& archive)
{
    const ZipReader zip(archive, kMaxArchiveTotal);
    return zip.isOpen() ? zip.names() : QStringList();
}

bool looksLikeActcraft(const QByteArray& head) noexcept
{
    return head.startsWith("PK\x03\x04");
}

} // namespace actcraft::formats
