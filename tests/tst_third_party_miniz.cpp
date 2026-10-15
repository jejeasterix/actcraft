#include <QByteArray>
#include <QTest>

#include <miniz.h>

namespace {

QByteArray zipOf(const QByteArray& text, const QByteArray& stored)
{
    mz_zip_archive zip{};
    if (!mz_zip_writer_init_heap(&zip, 0, 1024)) {
        return {};
    }
    mz_zip_writer_add_mem(&zip, "a.txt", text.constData(), static_cast<size_t>(text.size()),
                          static_cast<mz_uint>(MZ_BEST_COMPRESSION));
    mz_zip_writer_add_mem(&zip, "b.bin", stored.constData(), static_cast<size_t>(stored.size()),
                          static_cast<mz_uint>(MZ_NO_COMPRESSION));
    void* buffer = nullptr;
    size_t size = 0;
    QByteArray out;
    if (mz_zip_writer_finalize_heap_archive(&zip, &buffer, &size)) {
        out = QByteArray(static_cast<const char*>(buffer), static_cast<qsizetype>(size));
        mz_free(buffer);
    }
    mz_zip_writer_end(&zip);
    return out;
}

QByteArray extract(const QByteArray& archive, const char* name)
{
    mz_zip_archive zip{};
    if (!mz_zip_reader_init_mem(&zip, archive.constData(), static_cast<size_t>(archive.size()), 0)) {
        return {};
    }
    QByteArray out;
    const int index = mz_zip_reader_locate_file(&zip, name, nullptr, 0);
    mz_zip_archive_file_stat stat{};
    if (index >= 0 && mz_zip_reader_file_stat(&zip, static_cast<mz_uint>(index), &stat)) {
        out.resize(static_cast<qsizetype>(stat.m_uncomp_size));
        if (!mz_zip_reader_extract_to_mem(&zip, static_cast<mz_uint>(index), out.data(),
                                          static_cast<size_t>(out.size()), 0)) {
            out.clear();
        }
    }
    mz_zip_reader_end(&zip);
    return out;
}

} // namespace

// miniz vendu (DA3) : écriture en mémoire, relecture, archives reproductibles (MINIZ_NO_TIME).
class TstThirdPartyMiniz : public QObject
{
    Q_OBJECT

private slots:
    void roundTripInMemory()
    {
        const QByteArray text(5000, 'x');
        const QByteArray stored("\x89PNG\r\n\x1a\n", 8);
        const QByteArray archive = zipOf(text, stored);
        QVERIFY(archive.startsWith("PK\x03\x04"));
        QVERIFY(archive.size() < text.size()); // compressé
        QCOMPARE(extract(archive, "a.txt"), text);
        QCOMPARE(extract(archive, "b.bin"), stored);
        QVERIFY(extract(archive, "absent").isEmpty());
    }

    void sameInputGivesSameBytes()
    {
        const QByteArray a = zipOf("bonjour", "abc");
        QTest::qWait(1100); // une seconde de plus : sans MINIZ_NO_TIME, la date DOS changerait
        QCOMPARE(zipOf("bonjour", "abc"), a);
    }
};

QTEST_GUILESS_MAIN(TstThirdPartyMiniz)
#include "tst_third_party_miniz.moc"
