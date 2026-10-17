#include "support/flipchartwriter.h"
#include "support/peakmemory.h"

#include <actcraft/formats/bamboo.h>

#include <QTest>

#include <miniz.h>

using namespace actcraft::formats::flipchart;
using namespace Qt::StringLiterals;

namespace {

// Flux zlib de `total` octets nuls, compressé par blocs de 1 Mio : jamais `total` octets en mémoire.
QByteArray zlibOfZeros(qint64 total)
{
    mz_stream s{};
    mz_deflateInit(&s, MZ_BEST_COMPRESSION);
    const QByteArray zeros(1 << 20, '\0');
    QByteArray buffer(1 << 16, Qt::Uninitialized);
    QByteArray out;
    for (qint64 done = 0; done < total; done += zeros.size()) {
        const bool last = done + zeros.size() >= total;
        s.next_in = reinterpret_cast<const unsigned char*>(zeros.constData());
        s.avail_in = static_cast<unsigned int>(zeros.size());
        int status = MZ_OK;
        do {
            s.next_out = reinterpret_cast<unsigned char*>(buffer.data());
            s.avail_out = static_cast<unsigned int>(buffer.size());
            status = mz_deflate(&s, last ? MZ_FINISH : MZ_NO_FLUSH);
            out.append(buffer.constData(), buffer.size() - static_cast<qsizetype>(s.avail_out));
        } while (s.avail_in > 0 || (last && status != MZ_STREAM_END));
    }
    mz_deflateEnd(&s);
    return out;
}

} // namespace

// Conteneur Bamboo et index « culm » (format-flipchart.md § 2 et § 3), sur des octets écrits par fctest.
class TstFormatsBamboo : public QObject
{
    Q_OBJECT

private slots:
    void readsCompressedAndRawEntries()
    {
        const QByteArray data = fctest::container(
            {fctest::Entry(u"100.io"_s, QByteArray(3000, 'a')), fctest::Entry(u"115.png"_s, "brut", false)});
        QString error;
        const auto c = BambooContainer::parse(data, error);
        QVERIFY2(c, qPrintable(error));
        QCOMPARE(c->version(), quint16{7});
        QCOMPARE(c->names(), (QStringList{u"100.io"_s, u"115.png"_s}));
        QVERIFY(c->contains(u"115.png"_s));
        QVERIFY(!c->contains(u"absent"_s));
        QCOMPARE(c->read(u"100.io"_s, error).value_or(QByteArray()), QByteArray(3000, 'a'));
        QCOMPARE(c->read(u"115.png"_s, error).value_or(QByteArray()), QByteArray("brut"));
        QVERIFY(!c->read(u"absent"_s, error));
        QVERIFY(error.contains(u"absent"_s));
    }

    void nestedContainerIsAnEntry()
    {
        const QByteArray page = fctest::container({fctest::Entry(u"103.in"_s, "index")});
        const QByteArray data = fctest::container({fctest::Entry(u"103.fpg"_s, page)});
        QString error;
        const auto outer = BambooContainer::parse(data, error);
        QVERIFY(outer);
        const auto inner =
            BambooContainer::parse(outer->read(u"103.fpg"_s, error).value_or(QByteArray()), error);
        QVERIFY2(inner, qPrintable(error));
        QVERIFY(inner->contains(u"103.in"_s));
    }

    void namesAreUtf16()
    {
        const QByteArray data = fctest::container({fctest::Entry(u"élève.io"_s, "x")});
        QString error;
        QCOMPARE(BambooContainer::parse(data, error)->names(), QStringList{u"élève.io"_s});
    }

    void refusesBadOrTruncatedData()
    {
        QString error;
        QVERIFY(!BambooContainer::parse("pas un conteneur", error));
        QVERIFY(error.contains(u"signature"_s));
        const QByteArray data = fctest::container({fctest::Entry(u"100.io"_s, QByteArray(100, 'z'))});
        for (const qsizetype cut : {qsizetype{10}, qsizetype{40}, data.size() - 3}) {
            error.clear();
            QVERIFY2(!BambooContainer::parse(data.left(cut), error), qPrintable(QString::number(cut)));
            QVERIFY(!error.isEmpty());
        }
    }

    void sizeMismatchIsAnError()
    {
        // Entrée écrite à la main : taille annoncée 99, contenu de 3 octets.
        fctest::Bytes b;
        b.raw(QByteArray(" Bamboo ", 8)).u16(7).qstr(u"1.0"_s).raw(QByteArray(8, '\0'));
        const QByteArray stored = qCompress(QByteArray("abc"));
        b.qstr(u"x.io"_s).qstr(u""_s).qstr(u""_s).qstr(u""_s).u64(99).u8(1).u64(
            static_cast<quint64>(stored.size()));
        b.raw(stored);
        QString error;
        const auto c = BambooContainer::parse(b.data(), error);
        QVERIFY(c);
        QVERIFY(!c->read(u"x.io"_s, error));
        QVERIFY2(error.contains(u"annoncé"_s), qPrintable(error));
    }

    void oversizedEntryIsRefusedWithoutAllocating()
    {
        fctest::Bytes b;
        b.raw(QByteArray(" Bamboo ", 8)).u16(7).qstr(u"1.0"_s).raw(QByteArray(8, '\0'));
        b.qstr(u"x.io"_s).qstr(u""_s).qstr(u""_s).qstr(u""_s).u64(600ull * 1024 * 1024).u8(1).u64(8);
        b.u32(qToBigEndian(quint32{600u * 1024u * 1024u})).raw("zzzz");
        QString error;
        const auto c = BambooContainer::parse(b.data(), error);
        QVERIFY(c);
        QVERIFY(!c->read(u"x.io"_s, error));
        QVERIFY2(error.contains(u"volumineuse"_s), qPrintable(error));
    }

    void zlibBombStaysWithinTheAnnouncedSize()
    {
        const QByteArray stream = zlibOfZeros(256ll << 20); // ≈ 256 Kio compressés
        fctest::Bytes b;
        b.raw(QByteArray(" Bamboo ", 8)).u16(7).qstr(u"1.0"_s).raw(QByteArray(8, '\0'));
        b.qstr(u"bombe.io"_s).qstr(u""_s).qstr(u""_s).qstr(u""_s).u64(10).u8(1);
        b.u64(static_cast<quint64>(4 + stream.size()));
        b.u32(qToBigEndian(quint32{10})).raw(stream); // en-tête qCompress menteur : 10 octets
        QString error;
        const auto c = BambooContainer::parse(b.data(), error);
        QVERIFY(c);
        const qint64 before = peakMemoryBytes();
        QVERIFY(!c->read(u"bombe.io"_s, error));
        const qint64 rise = peakMemoryBytes() - before;
        QVERIFY2(rise < (32ll << 20), qPrintable(QStringLiteral("hausse du pic : %1 Mio").arg(rise >> 20)));
    }

    void decompressionStopsAtTheAnnouncedSize()
    {
        // Entrée piégée : 10 octets annoncés, flux qui en donnerait 4 Mio (bombe zlib en petit).
        fctest::Bytes b;
        b.raw(QByteArray(" Bamboo ", 8)).u16(7).qstr(u"1.0"_s).raw(QByteArray(8, '\0'));
        const QByteArray stored = qCompress(QByteArray(4 * 1024 * 1024, 'a'));
        b.qstr(u"bombe.io"_s)
            .qstr(u""_s)
            .qstr(u""_s)
            .qstr(u""_s)
            .u64(10)
            .u8(1)
            .u64(static_cast<quint64>(stored.size()));
        b.raw(stored);
        QString error;
        const auto c = BambooContainer::parse(b.data(), error);
        QVERIFY(c);
        QVERIFY(!c->read(u"bombe.io"_s, error));
        QVERIFY2(error.contains(u"annoncés"_s), qPrintable(error));
    }

    void indexKeepsTreeOrderAndReferences()
    {
        const fctest::Node root(
            101, 1, 100,
            {fctest::Node(103, 2, 102,
                          {fctest::Node(104, 29, 105), fctest::Node(118, 18, 119, {}, {110, 112})}),
             fctest::Node(203, 2, 202)});
        QString error;
        const auto tree = parseIndex(fctest::index(root), error);
        QVERIFY2(tree, qPrintable(error));
        QCOMPARE(tree->nodeId, quint32{101});
        QCOMPARE(tree->type, quint16{1});
        QCOMPARE(tree->storageName(), u"100.io"_s);
        QCOMPARE(tree->children.size(), std::size_t{2});
        const IndexNode& page = tree->children[0];
        QCOMPARE(page.nodeId, quint32{103});
        QCOMPARE(page.children.size(), std::size_t{2});
        QCOMPARE(page.children[1].type, quint16{18});
        QCOMPARE(page.children[1].refs, (std::vector<quint32>{110, 112}));
        QCOMPARE(tree->children[1].storageId, quint32{202});
    }

    void indexErrorsAreReported_data()
    {
        QTest::addColumn<QByteArray>("tail");
        QTest::addColumn<QString>("expected");
        QTest::newRow("vide") << QByteArray() << u"vide"_s;
        QTest::newRow("balise") << QByteArray("X") << u"balise"_s;
        QTest::newRow("fermeture") << QByteArray("n") << u"sans nœud"_s;
        fctest::Bytes two;
        two.tag('N').u32(1).u16(1).u32(1).tag('n').tag('N').u32(2).u16(1).u32(2).tag('n');
        QTest::newRow("deux racines") << two.data() << u"racines"_s;
        fctest::Bytes open;
        open.tag('N').u32(1).u16(1).u32(1);
        QTest::newRow("non fermé") << open.data() << u"fermé"_s;
    }

    void indexErrorsAreReported()
    {
        QFETCH(QByteArray, tail);
        QFETCH(QString, expected);
        fctest::Bytes b;
        b.raw(QByteArray("# culm #", 8)).u16(1).u16(7).raw(tail);
        QString error;
        QVERIFY(!parseIndex(b.data(), error));
        QVERIFY2(error.contains(expected), qPrintable(error));
        QVERIFY(!parseIndex("pas un index", error));
    }
};

QTEST_GUILESS_MAIN(TstFormatsBamboo)
#include "tst_formats_bamboo.moc"
