#include "support/richdocument.h"

#include <actcraft/core/edits.h>
#include <actcraft/formats/nativefile.h>
#include <actcraft/formats/nativejson.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>
#include <QtEndian>

#include <miniz.h>
#include <string>
#include <vector>

using namespace actcraft;
using namespace Qt::StringLiterals;

namespace {

QByteArray jsonOf(const core::Document& document)
{
    return QJsonDocument(formats::documentToJson(document)).toJson(QJsonDocument::Indented);
}

bool addMem(mz_zip_archive& zip, const std::string& name, const QByteArray& data)
{
    return mz_zip_writer_add_mem(&zip, name.c_str(), data.constData(), static_cast<size_t>(data.size()),
                                 static_cast<mz_uint>(MZ_DEFAULT_LEVEL)) != 0;
}

// Archive zip quelconque écrite par miniz : de quoi fabriquer celles qu'encodeActcraft n'écrirait jamais.
template <typename Fill> QByteArray writeZip(Fill fill)
{
    mz_zip_archive zip{};
    if (!mz_zip_writer_init_heap(&zip, 0, 0)) {
        return {};
    }
    void* buffer = nullptr;
    size_t size = 0;
    if (!fill(zip) || !mz_zip_writer_finalize_heap_archive(&zip, &buffer, &size)) {
        mz_zip_writer_end(&zip);
        return {};
    }
    QByteArray out(static_cast<const char*>(buffer), static_cast<qsizetype>(size));
    mz_free(buffer);
    mz_zip_writer_end(&zip);
    return out;
}

// Position de l'en-tête central (« PK\1\2 ») de l'entrée `name` ; -1 si absente.
qsizetype centralHeader(const QByteArray& zip, const QByteArray& name)
{
    const QByteArray signature("PK\x01\x02", 4);
    for (qsizetype at = zip.indexOf(signature); at >= 0 && at + 46 <= zip.size();
         at = zip.indexOf(signature, at + 1)) {
        const auto length = static_cast<qsizetype>(qFromLittleEndian<quint16>(zip.constData() + at + 28));
        if (zip.mid(at + 46, length) == name) {
            return at;
        }
    }
    return -1;
}

void setLe32(QByteArray& zip, qsizetype at, quint32 value)
{
    qToLittleEndian<quint32>(value, zip.data() + at);
}

quint16 le16(const QByteArray& zip, qsizetype at)
{
    return qFromLittleEndian<quint16>(zip.constData() + at);
}

quint32 le32(const QByteArray& zip, qsizetype at)
{
    return qFromLittleEndian<quint32>(zip.constData() + at);
}

// Octets d'une entrée de l'archive, extraits par miniz (pas par le lecteur testé) ; vide si absente.
QByteArray entryOf(const QByteArray& archive, const char* name)
{
    mz_zip_archive zip{};
    if (!mz_zip_reader_init_mem(&zip, archive.constData(), static_cast<size_t>(archive.size()), 0)) {
        return {};
    }
    size_t size = 0;
    void* data = mz_zip_reader_extract_file_to_heap(&zip, name, &size, 0);
    QByteArray out =
        data ? QByteArray(static_cast<const char*>(data), static_cast<qsizetype>(size)) : QByteArray();
    mz_free(data);
    mz_zip_reader_end(&zip);
    return out;
}

// Texte de l'objet JSON qui porte la clé `key` (guillemets compris), tel qu'il est écrit dans `text` : de
// son « { » à son « } », chaînes respectées. Vide si absent.
QByteArray enclosingObject(const QByteArray& text, const QByteArray& key)
{
    const qsizetype at = text.indexOf(key);
    std::vector<qsizetype> open;
    qsizetype start = -1;
    bool inString = false;
    for (qsizetype i = 0; at >= 0 && i < text.size(); ++i) {
        if (i == at) {
            if (open.empty()) {
                return {};
            }
            start = open.back();
        }
        const char c = text.at(i);
        if (inString) {
            if (c == '\\') {
                ++i;
            } else if (c == '"') {
                inString = false;
            }
        } else if (c == '"') {
            inString = true;
        } else if (c == '{') {
            open.push_back(i);
        } else if (c == '}' && !open.empty()) {
            if (open.back() == start) {
                return text.mid(start, i - start + 1);
            }
            open.pop_back();
        }
    }
    return {};
}

// Accès aux bornes une à une (pas d'agrégat partiel par position).
formats::ArchiveLimits limits(qint64 maxEntry, qint64 maxTotal, qint64 maxArchiveSize)
{
    formats::ArchiveLimits l;
    l.maxEntry = maxEntry;
    l.maxTotal = maxTotal;
    l.maxArchiveSize = maxArchiveSize;
    return l;
}

} // namespace

// Archive .actcraft (DA3) : document.json + ressources + miniatures ; aller-retour à empreinte égale. Chaque
// garde contre une archive hostile a son test, qui tombe si on la retire.
class TstFormatsNativeFile : public QObject
{
    Q_OBJECT

private slots:
    void roundTripKeepsTheFingerprintAndThumbnails()
    {
        const core::Document rich = richdoc::richDocument();
        const std::vector<QByteArray> thumbs{QByteArray("png-1"), QByteArray("png-2"), QByteArray("png-3")};
        QString error;
        const QByteArray archive = formats::encodeActcraft(rich, thumbs, &error);
        QVERIFY2(!archive.isEmpty(), qPrintable(error));
        QVERIFY(formats::looksLikeActcraft(archive));
        const formats::DecodeResult r = formats::decodeActcraft(archive);
        QVERIFY2(r.ok, qPrintable(r.error));
        QVERIFY2(r.warnings.isEmpty(), qPrintable(r.warnings.join(u'\n')));
        QCOMPARE(core::fingerprint(r.document), core::fingerprint(rich));
        QCOMPARE(r.thumbnails.size(), std::size_t{3});
        QCOMPARE(r.thumbnails[1], QByteArray("png-2"));
    }

    void entriesAreNamedAsDocumented()
    {
        const core::Document rich = richdoc::richDocument();
        const QByteArray archive =
            formats::encodeActcraft(rich, {QByteArray("a"), QByteArray("b"), QByteArray("c")});
        const QStringList names = formats::archiveEntryNames(archive);
        QCOMPARE(names.first(), u"document.json"_s);
        const QString png = QString::fromStdString(core::sha256Hex(richdoc::pngBytes()));
        const QString mp3 = QString::fromStdString(core::sha256Hex(richdoc::mp3Bytes()));
        QVERIFY(names.contains(u"resources/"_s + png + u".png"_s));
        QVERIFY(names.contains(u"resources/"_s + mp3 + u".mp3"_s));
        QVERIFY(names.contains(u"thumbnails/page-1.png"_s));
        QVERIFY(names.contains(u"thumbnails/page-3.png"_s));
        QCOMPARE(names.size(), 6);
    }

    void resourcesAreStoredVerbatim()
    {
        const core::Document rich = richdoc::richDocument();
        const QByteArray archive = formats::encodeActcraft(rich, {QByteArray("png-1")});
        QVERIFY(archive.contains(richdoc::pngBytes())); // octets d'origine lisibles par d'autres outils
        QVERIFY(archive.contains(richdoc::mp3Bytes()));
        // Méthode de chaque entrée : 0 (stockée) pour ressources et miniatures, 8 (deflate) pour
        // document.json. Un petit flux deflate peut garder les octets lisibles (bloc stocké) : seule la
        // méthode le distingue.
        const QStringList names = formats::archiveEntryNames(archive);
        QCOMPARE(names.size(), 4);
        for (const QString& name : names) {
            const qsizetype at = centralHeader(archive, name.toUtf8());
            QVERIFY(at >= 0);
            QCOMPARE(le16(archive, at + 10), quint16(name == u"document.json"_s ? 8 : 0));
        }
    }

    void encodingIsReproducible()
    {
        const core::Document rich = richdoc::richDocument();
        const QByteArray archive = formats::encodeActcraft(rich, {QByteArray("x")});
        QCOMPARE(archive, formats::encodeActcraft(rich, {QByteArray("x")}));
        // Heure et date DOS nulles dans chaque en-tête central et local : rien ne dépend du moment de
        // l'écriture (deux écritures dans la même seconde seraient égales même horodatées).
        for (const QString& name : formats::archiveEntryNames(archive)) {
            const qsizetype at = centralHeader(archive, name.toUtf8());
            QVERIFY(at >= 0);
            QCOMPARE(le32(archive, at + 12), quint32{0});
            const auto local = static_cast<qsizetype>(le32(archive, at + 42));
            QCOMPARE(le32(archive, local + 10), quint32{0});
        }
    }

    void missingThumbnailsAreAllowed()
    {
        const core::Document rich = richdoc::richDocument();
        const formats::DecodeResult r = formats::decodeActcraft(formats::encodeActcraft(rich, {}));
        QVERIFY(r.ok);
        QCOMPARE(r.thumbnails.size(), std::size_t{3}); // une case par page, vides
        QVERIFY(r.thumbnails[0].isEmpty());
    }

    // L2-C9, constat 6 : l'ouverture ne veut pas les miniatures. Non extraites, elles ne consomment pas le
    // budget de décompression avant les entrées inconnues, et restent des entrées connues du format.
    void skippedThumbnailsSpareTheBudget()
    {
        const core::Document pages{std::vector<core::Page>(2)};
        const QByteArray json = jsonOf(pages);
        const QByteArray thumbnail(100, 't');
        const QByteArray extra(100, 'x');
        const QByteArray archive = writeZip([&](mz_zip_archive& zip) {
            return addMem(zip, "document.json", json) && addMem(zip, "thumbnails/page-1.png", thumbnail) &&
                   addMem(zip, "thumbnails/page-2.png", thumbnail) && addMem(zip, "web/extra.bin", extra);
        });
        // Budget : document.json et 150 octets. Lues, la première miniature en prend 100 : il ne reste rien
        // pour l'entrée inconnue.
        const formats::ArchiveLimits budget = limits(1 << 20, json.size() + 150, 1 << 20);
        const formats::DecodeResult read = formats::decodeActcraft(archive, budget);
        QVERIFY2(read.ok, qPrintable(read.error));
        QCOMPARE(read.thumbnails.size(), std::size_t{2});
        QCOMPARE(read.thumbnails[0], thumbnail);
        QVERIFY(read.document.unknownEntries().empty());
        const formats::DecodeResult skipped =
            formats::decodeActcraft(archive, budget, formats::Thumbnails::Skip);
        QVERIFY2(skipped.ok, qPrintable(skipped.error));
        QVERIFY(skipped.thumbnails.empty());
        QCOMPARE(skipped.document.unknownEntries().size(), std::size_t{1});
        QCOMPARE(skipped.document.unknownEntries().at("web/extra.bin").data, extra);
        QVERIFY2(skipped.warnings.isEmpty(), qPrintable(skipped.warnings.join(u'\n')));
    }

    void totalDecompressionBudgetIsEnforced()
    {
        // Budget = document.json + 1 octet : chaque entrée tient seule sous le budget, seul le CUMUL le
        // dépasse dès la première ressource (sans le cumul, les deux ressources seraient lues et le test
        // passerait au rouge).
        const core::Document rich = richdoc::richDocument();
        const QByteArray archive = formats::encodeActcraft(rich, {});
        QVERIFY(formats::decodeActcraft(archive).ok);
        const qint64 json = jsonOf(rich).size();
        const formats::DecodeResult r = formats::decodeActcraft(archive, json + 1);
        QVERIFY2(
            r.ok,
            qPrintable(r.error)); // document.json lu ; les ressources refusées deviennent des avertissements
        QCOMPARE(r.document.resources().size(), std::size_t{0});
        // La cause est transmise (revue de L2-T6) : « trop volumineuse », non plus « absente ».
        const QString warnings = r.warnings.join(u'\n');
        QVERIFY2(warnings.contains(u"trop volumineuse"_s) && !warnings.contains(u"absente"_s),
                 qPrintable(warnings));
    }

    void failedExtractionsCountAgainstTheBudget()
    {
        // Deux miniatures de 1 Mio au CRC faux, puis une valide de 10 octets ; budget = document.json + 1 Mio
        // + 10. miniz ne vérifie le CRC qu'après avoir tout décompressé : une extraction ratée a coûté autant
        // qu'une réussie. Imputée avant d'extraire, la deuxième est refusée sans être décompressée.
        const core::Document pages{std::vector<core::Page>(3)};
        const QByteArray mib(1 << 20, 'm');
        QByteArray archive = formats::encodeActcraft(pages, {mib, mib, QByteArray(10, 't')});
        for (const char* name : {"thumbnails/page-1.png", "thumbnails/page-2.png"}) {
            const qsizetype at = centralHeader(archive, name);
            QVERIFY(at >= 0);
            setLe32(archive, at + 16, le32(archive, at + 16) ^ 1u); // CRC faux
        }
        const formats::DecodeResult r =
            formats::decodeActcraft(archive, jsonOf(pages).size() + (1 << 20) + 10);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.thumbnails[2], QByteArray(10, 't'));
        const QString warnings = r.warnings.join(u'\n');
        QVERIFY2(warnings.contains(u"page 1 illisible"_s), qPrintable(warnings));
        QVERIFY2(warnings.contains(u"page 2 trop volumineuse"_s), qPrintable(warnings));
    }

    // L2-C9, constat 7 : une ressource refusée par l'archive a UN avertissement, qui dit la cause. Avant, le
    // lecteur cherchait le texte « absente » de documentFromJson pour le remplacer : un libellé changé là-bas
    // laissait deux avertissements (le libellé changé, et la cause ajoutée).
    void refusedResourceHasOneWarningWithItsCause()
    {
        const core::Document rich = richdoc::richDocument();
        const QByteArray archive = formats::encodeActcraft(rich, {});
        const formats::DecodeResult r = formats::decodeActcraft(archive, jsonOf(rich).size() + 1);
        QVERIFY2(r.ok, qPrintable(r.error));
        for (const auto& [key, resource] : rich.resources()) {
            const QString k = QString::fromStdString(key);
            const QStringList about = r.warnings.filter(k);
            QCOMPARE(about.size(), 1);
            QVERIFY2(about.first().contains(u"trop volumineuse"_s), qPrintable(about.first()));
        }
    }

    void refusedResourceSaysWhy()
    {
        // Ressource au CRC faux : l'avertissement dit « illisible », pas « absente ».
        QByteArray archive = formats::encodeActcraft(richdoc::richDocument(), {});
        const QString png = QString::fromStdString(core::sha256Hex(richdoc::pngBytes()));
        const qsizetype at = centralHeader(archive, (u"resources/"_s + png + u".png"_s).toUtf8());
        QVERIFY(at >= 0);
        setLe32(archive, at + 16, le32(archive, at + 16) ^ 1u);
        const formats::DecodeResult r = formats::decodeActcraft(archive);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.document.resources().size(), std::size_t{1});
        const QString warnings = r.warnings.join(u'\n');
        QVERIFY2(warnings.contains(png + u" illisible"_s) && !warnings.contains(u"absente"_s),
                 qPrintable(warnings));
    }

    void documentJsonMustBeAnObject()
    {
        const QByteArray zip =
            writeZip([](mz_zip_archive& z) { return addMem(z, "document.json", QByteArray("[]")); });
        const formats::DecodeResult r = formats::decodeActcraft(zip);
        QVERIFY(!r.ok);
        QVERIFY2(r.error.contains(u"objet"_s), qPrintable(r.error));
    }

    void caseOnlyDuplicatesAreRefused()
    {
        // « document.json » et « DOCUMENT.JSON » : un outil qui ignore la casse ne saurait lequel lire.
        const QByteArray json = jsonOf(richdoc::richDocument());
        const QByteArray zip = writeZip([&](mz_zip_archive& z) {
            return addMem(z, "document.json", json) && addMem(z, "DOCUMENT.JSON", json);
        });
        const formats::DecodeResult r = formats::decodeActcraft(zip);
        QVERIFY(!r.ok);
        QVERIFY2(r.error.contains(u"double"_s), qPrintable(r.error));
    }

    void encodingRefusesWhatDecodingWouldRefuse()
    {
        // Chaque borne de la lecture, réduite : l'écriture refuse un octet de trop, accepte pile la borne, et
        // l'archive acceptée se relit sous les mêmes bornes. Sinon, un fichier enregistré ne se rouvrirait
        // pas.
        const core::Document rich = richdoc::richDocument();
        const qint64 json = jsonOf(rich).size();
        const qint64 resources = richdoc::pngBytes().size() + richdoc::mp3Bytes().size();
        const qint64 big = formats::kMaxArchiveSize;
        QString error;
        const auto roundTrips = [&](const formats::ArchiveLimits& l, const std::vector<QByteArray>& thumbs) {
            const QByteArray archive = formats::encodeActcraft(rich, thumbs, &error, l);
            const formats::DecodeResult r = formats::decodeActcraft(archive, l);
            return !archive.isEmpty() && r.ok && r.warnings.isEmpty();
        };
        // Une entrée (ici une miniature) au-delà de la borne par entrée, fixée à la taille de document.json.
        const std::vector<QByteArray> tooBig{QByteArray(static_cast<qsizetype>(json + 1), 'p')};
        QVERIFY(formats::encodeActcraft(rich, tooBig, &error, limits(json, big, big)).isEmpty());
        QVERIFY2(error.contains(u"thumbnails/page-1.png"_s) && error.contains(u"trop volumineuse"_s),
                 qPrintable(error));
        const std::vector<QByteArray> justFits{QByteArray(static_cast<qsizetype>(json), 'p')};
        QVERIFY2(roundTrips(limits(json, big, big), justFits), qPrintable(error));
        // Le total décompressé : document.json + les deux ressources.
        QVERIFY(formats::encodeActcraft(rich, {}, &error, limits(big, json + resources - 1, big)).isEmpty());
        QVERIFY2(error.contains(u"au total"_s), qPrintable(error));
        QVERIFY2(roundTrips(limits(big, json + resources, big), {}), qPrintable(error));
        // La taille de l'archive.
        const qint64 size = formats::encodeActcraft(rich, {}).size();
        QVERIFY(formats::encodeActcraft(rich, {}, &error, limits(big, big, size - 1)).isEmpty());
        QVERIFY2(error.contains(u"archive trop volumineuse"_s), qPrintable(error));
        QVERIFY2(roundTrips(limits(big, big, size), {}), qPrintable(error));
        // La lecture, elle, refuse l'archive trop grande.
        const formats::DecodeResult r =
            formats::decodeActcraft(formats::encodeActcraft(rich, {}), limits(big, big, size - 1));
        QVERIFY(!r.ok);
        QVERIFY2(r.error.contains(u"trop volumineuse"_s), qPrintable(r.error));
    }

    void oversizedEntryIsRefusedOnItsAnnouncedSize()
    {
        // document.json annoncé à 512 Mio + 1 pour 2 octets compressés : refusé sur sa taille annoncée, avant
        // toute allocation. Le budget total (2 Gio) le laisserait passer : seule la borne par entrée le
        // refuse.
        const QByteArray zip = writeZip([](mz_zip_archive& z) {
            static const char emptyDeflate[] = {0x03, 0x00};
            return mz_zip_writer_add_mem_ex(&z, "document.json", emptyDeflate, sizeof(emptyDeflate), nullptr,
                                            0, MZ_ZIP_FLAG_COMPRESSED_DATA,
                                            static_cast<mz_uint64>(formats::kMaxArchiveEntry) + 1, 0) != 0;
        });
        QVERIFY(!zip.isEmpty());
        const formats::DecodeResult r = formats::decodeActcraft(zip);
        QVERIFY(!r.ok);
        QVERIFY2(r.error.contains(u"volumineux"_s), qPrintable(r.error));
    }

    void entryLargerThanAnnouncedIsRefused()
    {
        // Le vrai document.json compressé, annoncé à 64 octets : la décompression s'arrête au bout du tampon
        // de la taille annoncée (jamais plus) et l'entrée est refusée. Témoin : le même flux annoncé à sa
        // vraie taille.
        const QByteArray json = jsonOf(richdoc::richDocument());
        size_t packedSize = 0;
        void* packed = tdefl_compress_mem_to_heap(json.constData(), static_cast<size_t>(json.size()),
                                                  &packedSize, TDEFL_DEFAULT_MAX_PROBES);
        QVERIFY(packed);
        const QByteArray deflate(static_cast<const char*>(packed), static_cast<qsizetype>(packedSize));
        mz_free(packed);
        const auto announced = [&](qsizetype size) {
            const auto crc = static_cast<mz_uint32>(
                mz_crc32(MZ_CRC32_INIT, reinterpret_cast<const unsigned char*>(json.constData()),
                         static_cast<size_t>(size)));
            return writeZip([&](mz_zip_archive& z) {
                return mz_zip_writer_add_mem_ex(
                           &z, "document.json", deflate.constData(), static_cast<size_t>(deflate.size()),
                           nullptr, 0, MZ_ZIP_FLAG_COMPRESSED_DATA, static_cast<mz_uint64>(size), crc) != 0;
            });
        };
        const formats::DecodeResult whole = formats::decodeActcraft(announced(json.size()));
        QVERIFY2(whole.ok, qPrintable(whole.error));
        const formats::DecodeResult r = formats::decodeActcraft(announced(64));
        QVERIFY(!r.ok);
        QVERIFY2(r.error.contains(u"document.json"_s), qPrintable(r.error));
    }

    void directoryEntryIsNotReadAsData()
    {
        // Attribut « dossier » (DOS) sur une miniature : miniz rend « réussi » sans rien écrire et le tampon
        // resterait non initialisé. L'entrée est écartée avec un avertissement.
        QByteArray archive = formats::encodeActcraft(richdoc::richDocument(), {QByteArray("png-1")});
        const qsizetype at = centralHeader(archive, "thumbnails/page-1.png");
        QVERIFY(at >= 0);
        setLe32(archive, at + 38, 0x10); // attributs externes
        const formats::DecodeResult r = formats::decodeActcraft(archive);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.thumbnails[0], QByteArray());
        QVERIFY2(r.warnings.join(u' ').contains(u"miniature"_s), qPrintable(r.warnings.join(u'\n')));
    }

    void zip64EntryWithoutCompressedBytesIsRefused()
    {
        // Champ zip64 : 1 Mio annoncé, 0 octet compressé. miniz ne vérifie la cohérence des deux tailles que
        // hors zip64, puis rend « réussi » sans rien écrire : l'entrée est refusée avant d'allouer.
        QByteArray extra(20, '\0');
        qToLittleEndian<quint16>(0x0001, extra.data());       // identifiant du champ zip64
        qToLittleEndian<quint16>(16, extra.data() + 2);       // longueur
        qToLittleEndian<quint64>(1u << 20, extra.data() + 4); // taille décompressée
        qToLittleEndian<quint64>(0, extra.data() + 12);       // taille compressée
        const QByteArray json = jsonOf(richdoc::richDocument());
        QByteArray zip = writeZip([&](mz_zip_archive& z) {
            return addMem(z, "document.json", json) &&
                   mz_zip_writer_add_mem_ex_v2(&z, "thumbnails/page-1.png", "png-1", 5, nullptr, 0, 0, 0, 0,
                                               nullptr, nullptr, 0, extra.constData(),
                                               static_cast<mz_uint>(extra.size())) != 0;
        });
        const qsizetype at = centralHeader(zip, "thumbnails/page-1.png");
        QVERIFY(at >= 0);
        setLe32(zip, at + 20, 0xFFFFFFFFu); // tailles : voir le champ zip64
        setLe32(zip, at + 24, 0xFFFFFFFFu);
        const formats::DecodeResult r = formats::decodeActcraft(zip);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.thumbnails[0], QByteArray());
        QVERIFY2(r.warnings.join(u' ').contains(u"miniature"_s), qPrintable(r.warnings.join(u'\n')));
    }

    void entryNamesMatchExactly()
    {
        // Le format nomme « document.json » : ni une autre casse (la recherche de miniz par défaut l'ignore),
        // ni un chemin qui s'y ramènerait.
        const QByteArray json = jsonOf(richdoc::richDocument());
        for (const char* name : {"DOCUMENT.JSON", "x/../document.json"}) {
            const QByteArray zip = writeZip([&](mz_zip_archive& z) { return addMem(z, name, json); });
            QVERIFY(!zip.isEmpty());
            const formats::DecodeResult r = formats::decodeActcraft(zip);
            QVERIFY2(!r.ok, name);
            QVERIFY2(r.error.contains(u"document.json"_s), qPrintable(r.error));
        }
    }

    void duplicateEntriesAreRefused()
    {
        // Deux « document.json » : lequel lire ? Un autre outil lirait peut-être l'autre. Archive refusée.
        const QByteArray rich = jsonOf(richdoc::richDocument());
        const QByteArray empty = jsonOf(core::Document{});
        const QByteArray zip = writeZip([&](mz_zip_archive& z) {
            return addMem(z, "document.json", rich) && addMem(z, "document.json", empty);
        });
        QVERIFY(!zip.isEmpty());
        const formats::DecodeResult r = formats::decodeActcraft(zip);
        QVERIFY(!r.ok);
        QVERIFY2(r.error.contains(u"double"_s), qPrintable(r.error));
        QVERIFY(formats::archiveEntryNames(zip).isEmpty());
    }

    void entryCountIsBounded()
    {
        // Une entrée de plus que la borne : archive refusée avant d'indexer les noms.
        const QByteArray json = jsonOf(core::Document{});
        const QByteArray zip = writeZip([&](mz_zip_archive& z) {
            bool ok = addMem(z, "document.json", json);
            for (std::size_t i = 1; ok && i <= formats::kMaxArchiveEntryCount; ++i) {
                ok = addMem(z, "vide-" + std::to_string(i), QByteArray());
            }
            return ok;
        });
        QVERIFY(!zip.isEmpty());
        const formats::DecodeResult r = formats::decodeActcraft(zip);
        QVERIFY(!r.ok);
        QVERIFY2(r.error.contains(u"entrées"_s), qPrintable(r.error));
    }

    void encodingRefusesMoreEntriesThanDecodingAccepts()
    {
        // document.json + 65 535 miniatures = une entrée de trop : l'archive serait refusée à la relecture.
        QString error;
        const std::vector<QByteArray> tooMany(formats::kMaxArchiveEntryCount, QByteArray("t"));
        QVERIFY(formats::encodeActcraft(core::Document{}, tooMany, &error).isEmpty());
        QVERIFY2(error.contains(u"entrées"_s), qPrintable(error));
        // Pile la borne : écrite et relue.
        const std::vector<QByteArray> justEnough(formats::kMaxArchiveEntryCount - 1, QByteArray("t"));
        const QByteArray archive = formats::encodeActcraft(core::Document{}, justEnough, &error);
        QVERIFY2(!archive.isEmpty(), qPrintable(error));
        const formats::DecodeResult r = formats::decodeActcraft(archive);
        QVERIFY2(r.ok, qPrintable(r.error));
    }

    void garbageIsRefusedWithAMessage()
    {
        QVERIFY(!formats::looksLikeActcraft("Bamboo"));
        const formats::DecodeResult r = formats::decodeActcraft(QByteArray("PK\x03\x04 pas un zip", 16));
        QVERIFY(!r.ok);
        QVERIFY(!r.error.isEmpty());
        QVERIFY(!formats::decodeActcraft(QByteArray()).ok);
    }

    // S31 (L2-C8) : paperboard d'une version plus récente, écrit ici à la main (jamais par encodeActcraft) :
    // une forme, un objet « equation » qui cite une image que rien d'autre ne cite, une ligne ; des clés
    // inconnues sur la ligne, la page et le document. Ouvert, un trait ajouté, enregistré dans une autre
    // archive, rouvert : l'objet inconnu a les mêmes octets dans document.json, la même place, sa ressource ;
    // les clés inconnues sont là.
    void s31UnknownObjectSurvivesOpenEditAndSave()
    {
        const QByteArray png = richdoc::pngBytes();
        const QString key = QString::fromStdString(core::sha256Hex(png));
        const QJsonObject shape{{u"type"_s, u"shape"_s},
                                {u"id"_s, 1},
                                {u"layer"_s, u"middle"_s},
                                {u"shape"_s, u"rectangle"_s},
                                {u"bounds"_s, QJsonArray{10, 10, 50, 50}}};
        const QJsonObject unknown{{u"type"_s, u"equation"_s},
                                  {u"id"_s, 2},
                                  {u"layer"_s, u"middle"_s},
                                  {u"transform"_s, QJsonArray{1, 0, 0, 1, 100, 100}},
                                  {u"bounds"_s, QJsonArray{0, 0, 120, 40}},
                                  {u"latex"_s, u"x^2 + 1"_s},
                                  {u"render"_s, key},
                                  {u"style"_s, QJsonObject{{u"size"_s, 18}}}};
        const QJsonObject line{{u"type"_s, u"line"_s},
                               {u"id"_s, 3},
                               {u"layer"_s, u"middle"_s},
                               {u"points"_s, QJsonArray{0, 0, 10, 10}},
                               {u"dashOffset"_s, 2}};
        const QJsonObject page{{u"width"_s, 1024},
                               {u"height"_s, 768},
                               {u"objects"_s, QJsonArray{shape, unknown, line}},
                               {u"zones"_s, QJsonArray{QJsonObject{{u"name"_s, u"haut"_s}}}}};
        const QJsonObject root{
            {u"format"_s, u"actcraft-document/1.3"_s},
            {u"title"_s, u"Plus récent"_s},
            {u"nextId"_s, 4},
            {u"resources"_s, QJsonArray{QJsonObject{{u"key"_s, key}, {u"extension"_s, u"png"_s}}}},
            {u"pages"_s, QJsonArray{page}},
            {u"restrictions"_s, QJsonObject{{u"export"_s, false}}}};
        const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Indented);
        const std::string resourceName = "resources/" + key.toStdString() + ".png";
        const QByteArray archive = writeZip([&](mz_zip_archive& zip) {
            return addMem(zip, "document.json", json) && addMem(zip, resourceName, png);
        });
        QVERIFY(!archive.isEmpty());

        formats::DecodeResult opened = formats::decodeActcraft(archive);
        QVERIFY2(opened.ok, qPrintable(opened.error));
        QVERIFY2(opened.warnings.join(u' ').contains(u"equation"_s), qPrintable(opened.warnings.join(u'\n')));
        QVERIFY(std::get_if<core::UnknownObject>(&opened.document.page(0).objects[1]));
        core::Stroke stroke;
        stroke.points = {{1, 1, 0.5f}, {20, 20, 0.5f}};
        core::PageObject added = stroke;
        added.common.id = opened.document.allocateId();
        core::insertAtTopOfLayer(opened.document.page(0), added);
        QString error;
        const QByteArray saved = formats::encodeActcraft(opened.document, {}, &error);
        QVERIFY2(!saved.isEmpty(), qPrintable(error));

        // Octets JSON de l'objet inconnu identiques à l'entrée, dans document.json lui-même.
        const QByteArray savedJson = entryOf(saved, "document.json");
        const QByteArray before = enclosingObject(json, "\"latex\"");
        QVERIFY(!before.isEmpty());
        QCOMPARE(enclosingObject(savedJson, "\"latex\""), before);
        // Même place (entre la forme et la ligne, dans sa couche), le trait ajouté au-dessus de la ligne.
        const formats::DecodeResult reopened = formats::decodeActcraft(saved);
        QVERIFY2(reopened.ok, qPrintable(reopened.error));
        const core::Page& back = reopened.document.page(0);
        QCOMPARE(back.objects.size(), std::size_t{4});
        const std::vector<core::ObjectId> ids{1, 2, 3, 4};
        for (std::size_t i = 0; i < ids.size(); ++i) {
            QCOMPARE(back.objects[i].common.id, ids[i]);
        }
        QVERIFY(std::get_if<core::UnknownObject>(&back.objects[1]));
        QCOMPARE(back.objects[1].common.layer, core::Layer::Middle);
        QCOMPARE(core::depthInLayer(back, 2), std::size_t{1});
        // Ressource citée par le seul objet inconnu : toujours dans l'archive, octets d'origine.
        QVERIFY(formats::archiveEntryNames(saved).contains(QString::fromStdString(resourceName)));
        QVERIFY(reopened.document.resource(key.toStdString()));
        QCOMPARE(reopened.document.resource(key.toStdString())->data, png);
        // Clés inconnues du document, de la page et de la ligne.
        const QJsonObject savedRoot = QJsonDocument::fromJson(savedJson).object();
        const QJsonObject savedPage = savedRoot.value(u"pages"_s).toArray().at(0).toObject();
        QCOMPARE(savedRoot.value(u"restrictions"_s), root.value(u"restrictions"_s));
        QCOMPARE(savedPage.value(u"zones"_s), page.value(u"zones"_s));
        QCOMPARE(savedPage.value(u"objects"_s).toArray().at(2).toObject().value(u"dashOffset"_s).toInt(), 2);
    }

    // Le texte gardé d'un objet inconnu est dans document.json : borné comme lui (ArchiveLimits), à la
    // lecture comme à l'écriture.
    void unknownObjectsCountAgainstTheDocumentJsonLimit()
    {
        core::UnknownObject u;
        u.type = "equation";
        u.json = "{\"latex\":\"" + std::string(4096, 'x') + "\",\"type\":\"equation\"}";
        core::PageObject o = u;
        o.common.id = 1;
        core::Page p;
        p.objects.push_back(o);
        const core::Document big(std::vector<core::Page>{p});
        const formats::ArchiveLimits small = limits(2048, 1 << 20, 1 << 20);
        QString error;
        QVERIFY(formats::encodeActcraft(big, {}, &error, small).isEmpty());
        QVERIFY2(error.contains(u"document.json"_s), qPrintable(error));
        const QByteArray archive = formats::encodeActcraft(big, {});
        QVERIFY(!archive.isEmpty());
        const formats::DecodeResult r = formats::decodeActcraft(archive, small);
        QVERIFY(!r.ok);
        QVERIFY2(r.error.contains(u"document.json"_s), qPrintable(r.error));
        QVERIFY(formats::decodeActcraft(archive).ok);
    }

    // L2-C8 : une entrée d'archive inconnue (fichier d'une version plus récente) est gardée, octets
    // identiques, et réécrite ; un nom dangereux (« .. », absolu, barre inverse) est écarté et jamais réécrit
    // ; un dossier n'a pas d'octets ; ce que le format nomme sans le lire (miniature en trop) n'est pas
    // gardé.
    void unknownArchiveEntriesAreKeptVerbatim()
    {
        const QByteArray json = jsonOf(core::Document{});
        QByteArray font(300, '\0');
        font[7] = '\x7F';
        font.append("fin");
        const QByteArray html = QStringLiteral("<p>Bonjour é</p>").toUtf8();
        QByteArray archive = writeZip([&](mz_zip_archive& zip) {
            return addMem(zip, "document.json", json) && addMem(zip, "web/index.html", html) &&
                   addMem(zip, "fonts/serif.ttf", font) && addMem(zip, "web/", QByteArray()) &&
                   addMem(zip, "thumbnails/page-9.png", QByteArray("x")) &&
                   addMem(zip, "../evil.txt", QByteArray("mal")) &&
                   addMem(zip, "Xabs.txt", QByteArray("mal")) && addMem(zip, "aYb.txt", QByteArray("mal"));
        });
        QVERIFY(!archive.isEmpty());
        archive.replace("Xabs.txt", "/abs.txt"); // noms local et central, même longueur
        archive.replace("aYb.txt", "a\\b.txt");
        const formats::DecodeResult opened = formats::decodeActcraft(archive);
        QVERIFY2(opened.ok, qPrintable(opened.error));
        const auto& kept = opened.document.unknownEntries();
        QCOMPARE(kept.size(), std::size_t{2});
        QCOMPARE(kept.at("web/index.html").data, html);
        QCOMPARE(kept.at("fonts/serif.ttf").data, font);
        const QString warnings = opened.warnings.join(u'\n');
        QVERIFY2(warnings.contains(u"../evil.txt"_s) && warnings.contains(u"« /abs.txt »"_s) &&
                     warnings.contains(u"a\\b.txt"_s),
                 qPrintable(warnings));
        // La miniature d'une page qui n'existe pas est perdue à l'enregistrement : avertissement.
        QVERIFY2(warnings.contains(u"thumbnails/page-9.png"_s), qPrintable(warnings));
        QCOMPARE(opened.warnings.size(), 4);
        QString error;
        const QByteArray saved = formats::encodeActcraft(opened.document, {}, &error);
        QVERIFY2(!saved.isEmpty(), qPrintable(error));
        QCOMPARE(formats::archiveEntryNames(saved),
                 (QStringList{u"document.json"_s, u"fonts/serif.ttf"_s, u"web/index.html"_s}));
        QCOMPARE(entryOf(saved, "web/index.html"), html);
        QCOMPARE(entryOf(saved, "fonts/serif.ttf"), font);
        const formats::DecodeResult reopened = formats::decodeActcraft(saved);
        QVERIFY2(reopened.ok && reopened.warnings.isEmpty(), qPrintable(reopened.warnings.join(u'\n')));
        QCOMPARE(reopened.document.unknownEntries().size(), std::size_t{2});
        QCOMPARE(core::fingerprint(reopened.document), core::fingerprint(opened.document));
    }

    // Une entrée inconnue a les bornes d'une ressource : taille par entrée et budget total à la lecture
    // (écartée avec un avertissement), même borne à l'écriture (refus).
    void unknownArchiveEntriesAreBoundedLikeResources()
    {
        const QByteArray json = jsonOf(core::Document{});
        const QByteArray big(4096, 'b');
        const QByteArray archive = writeZip([&](mz_zip_archive& zip) {
            return addMem(zip, "document.json", json) && addMem(zip, "web/big.bin", big) &&
                   addMem(zip, "web/small.txt", QByteArray("ok"));
        });
        const formats::DecodeResult perEntry =
            formats::decodeActcraft(archive, limits(2048, 1 << 20, 1 << 20));
        QVERIFY(perEntry.ok);
        QCOMPARE(perEntry.document.unknownEntries().size(), std::size_t{1});
        QVERIFY(perEntry.document.unknownEntries().count("web/small.txt"));
        QVERIFY2(perEntry.warnings.join(u' ').contains(u"web/big.bin"_s),
                 qPrintable(perEntry.warnings.join(u'\n')));
        const formats::DecodeResult total =
            formats::decodeActcraft(archive, limits(1 << 20, json.size() + 1000, 1 << 20));
        QVERIFY(total.ok);
        QCOMPARE(total.document.unknownEntries().size(), std::size_t{1});
        QVERIFY(total.document.unknownEntries().count("web/small.txt"));
        core::Document withBig;
        withBig.setUnknownEntry("web/big.bin", big);
        QString error;
        QVERIFY(formats::encodeActcraft(withBig, {}, &error, limits(2048, 1 << 20, 1 << 20)).isEmpty());
        QVERIFY2(error.contains(u"web/big.bin"_s), qPrintable(error));
    }

    // L2-C8 : noms d'entrées inconnues. Chaque branche du filtre a sa ligne ; la lecture écarte avec un
    // avertissement, l'écriture refuse (rien n'est écrit en silence).
    void unknownEntryNameFilter_data()
    {
        QTest::addColumn<QByteArray>("name");
        QTest::addColumn<bool>("accepted");
        QTest::newRow("plain") << QByteArray("web/index.html") << true;
        QTest::newRow("utf8") << QByteArray("dossier/é.txt") << true;
        QTest::newRow("exactly-1024-bytes") << QByteArray(1024, 'a') << true;
        QTest::newRow("1025-bytes") << QByteArray(1025, 'a') << false;
        QTest::newRow("invalid-utf8") << QByteArray("a\xFF"
                                                    "\xFE"
                                                    "b.txt")
                                      << false;
        QTest::newRow("colon-ntfs-stream") << QByteArray("a.txt:x") << false;
        QTest::newRow("control-0x01") << QByteArray("a\x01"
                                                    ".txt")
                                      << false;
        QTest::newRow("control-0x1F") << QByteArray("a\x1F"
                                                    ".txt")
                                      << false;
        QTest::newRow("delete-0x7F") << QByteArray("a\x7F"
                                                   ".txt")
                                     << false;
        QTest::newRow("empty-segment") << QByteArray("a//b.txt") << false;
        QTest::newRow("dot-segment") << QByteArray("a/./b.txt") << false;
        QTest::newRow("dotdot-segment") << QByteArray("a/../b.txt") << false;
    }

    void unknownEntryNameFilter()
    {
        QFETCH(QByteArray, name);
        QFETCH(bool, accepted);
        const QByteArray json = jsonOf(core::Document{});
        const QByteArray archive = writeZip([&](mz_zip_archive& zip) {
            return addMem(zip, "document.json", json) && addMem(zip, name.toStdString(), QByteArray("x"));
        });
        QVERIFY(!archive.isEmpty());
        const formats::DecodeResult opened = formats::decodeActcraft(archive);
        QVERIFY2(opened.ok, qPrintable(opened.error));
        QCOMPARE(opened.document.unknownEntries().size(), accepted ? std::size_t{1} : std::size_t{0});
        QCOMPARE(opened.warnings.size(), accepted ? 0 : 1);

        core::Document document;
        document.setUnknownEntry(name.toStdString(), QByteArray("x"));
        QString error;
        const QByteArray saved = formats::encodeActcraft(document, {}, &error);
        QCOMPARE(!saved.isEmpty(), accepted);
        if (!accepted) {
            QVERIFY2(error.contains(u"nom refusé"_s), qPrintable(error));
        }
    }

    // Les noms du format ne s'écrivent jamais comme entrée inconnue (ils doubleraient document.json, une
    // ressource ou une miniature) ; deux noms que la casse seule distingue non plus (la lecture refuserait
    // l'archive).
    void unknownEntriesNeverDuplicateAFormatEntryOnWrite_data()
    {
        QTest::addColumn<QByteArray>("name");
        QTest::newRow("document") << QByteArray("document.json");
        QTest::newRow("document-other-case") << QByteArray("Document.JSON");
        QTest::newRow("resource") << QByteArray("resources/abc.png");
        QTest::newRow("resource-other-case") << QByteArray("Resources/abc.png");
        QTest::newRow("thumbnail") << QByteArray("thumbnails/page-1.png");
        QTest::newRow("thumbnail-other-case") << QByteArray("THUMBNAILS/page-1.png");
    }

    void unknownEntriesNeverDuplicateAFormatEntryOnWrite()
    {
        QFETCH(QByteArray, name);
        core::Document document;
        document.setUnknownEntry(name.toStdString(), QByteArray("x"));
        QString error;
        QVERIFY(formats::encodeActcraft(document, {QByteArray("t")}, &error).isEmpty());
        QVERIFY2(error.contains(u"nom réservé"_s), qPrintable(error));
    }

    void unknownEntriesDifferingOnlyByCaseAreRefusedOnWrite()
    {
        core::Document document;
        document.setUnknownEntry("web/A.txt", QByteArray("1"));
        document.setUnknownEntry("web/a.txt", QByteArray("2"));
        QString error;
        QVERIFY(formats::encodeActcraft(document, {}, &error).isEmpty());
        QVERIFY2(error.contains(u"en double"_s), qPrintable(error));
    }

    // L2-C8 : les entrées inconnues sont réécrites compressées. 100 Ko d'octets identiques tiennent dans une
    // archive bornée à 20 Ko ; stockées, elles la dépasseraient (la lecture en accepte maxTotal
    // décompressés).
    void unknownEntriesAreWrittenCompressed()
    {
        core::Document document;
        const QByteArray text(100000, 'a');
        document.setUnknownEntry("web/page.txt", text);
        QString error;
        const QByteArray saved =
            formats::encodeActcraft(document, {}, &error, limits(1 << 20, 1 << 20, 20000));
        QVERIFY2(!saved.isEmpty(), qPrintable(error));
        QCOMPARE(entryOf(saved, "web/page.txt"), text);
        QCOMPARE(formats::decodeActcraft(saved).document.unknownEntries().at("web/page.txt").data, text);
    }

    // L2-C8 : une entrée au nom du format que cette version n'a pas lue (ressource non déclarée, miniature
    // d'une page absente) est perdue à l'enregistrement : avertissement. Les entrées lues n'en ont pas.
    void unreadFormatEntriesAreReported()
    {
        core::Document document;
        const std::string key = document.addResource(QByteArray("png-bytes"), "png");
        const QByteArray base = formats::encodeActcraft(document, {QByteArray("t")});
        QVERIFY(formats::decodeActcraft(base).warnings.isEmpty());
        const QByteArray json = entryOf(base, "document.json");
        const QByteArray archive = writeZip([&](mz_zip_archive& zip) {
            return addMem(zip, "document.json", json) &&
                   addMem(zip, "resources/" + key + ".png", QByteArray("png-bytes")) &&
                   addMem(zip, "thumbnails/page-1.png", QByteArray("t")) &&
                   addMem(zip, "resources/orpheline.png", QByteArray("o")) &&
                   addMem(zip, "thumbnails/page-2.png", QByteArray("t2"));
        });
        const formats::DecodeResult r = formats::decodeActcraft(archive);
        QVERIFY2(r.ok, qPrintable(r.error));
        const QString warnings = r.warnings.join(u'\n');
        QVERIFY2(warnings.contains(u"resources/orpheline.png"_s) &&
                     warnings.contains(u"thumbnails/page-2.png"_s),
                 qPrintable(warnings));
        QCOMPARE(r.warnings.size(), 2);
    }

    void archiveWithoutDocumentJsonIsRefused()
    {
        // Zip valide dont l'entrée « document.json » porte un autre nom de même longueur.
        core::Document empty;
        QByteArray archive = formats::encodeActcraft(empty, {QByteArray("t")});
        archive.replace("document.json", "documentXjson"); // nom local et nom central modifiés ensemble
        const formats::DecodeResult r = formats::decodeActcraft(archive);
        QVERIFY(!r.ok);
        QVERIFY2(r.error.contains(u"document.json"_s), qPrintable(r.error));
    }
};

QTEST_GUILESS_MAIN(TstFormatsNativeFile)
#include "tst_formats_nativefile.moc"
