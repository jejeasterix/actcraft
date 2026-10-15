#include "support/richdocument.h"

#include <actcraft/formats/nativefile.h>
#include <actcraft/formats/nativejson.h>

#include <QJsonDocument>
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
