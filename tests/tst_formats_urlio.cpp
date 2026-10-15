#include <actcraft/formats/urlio.h>

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <algorithm>
#include <cstring>
#include <utility>

using namespace actcraft;
using namespace Qt::StringLiterals;

namespace {

// Comme un content:// : séquentiel, taille 0 annoncée, atEnd() vrai avant toute lecture. `failAt` : position
// où readData rend -1 (erreur de lecture), -1 pour jamais.
class SequentialDevice final : public QIODevice
{
public:
    explicit SequentialDevice(QByteArray data, qint64 failAt = -1) : m_data(std::move(data)), m_failAt(failAt)
    {}
    bool isSequential() const override { return true; }

protected:
    qint64 readData(char* out, qint64 max) override
    {
        if (m_failAt >= 0 && m_pos >= m_failAt) {
            return -1;
        }
        qint64 n = std::min(max, static_cast<qint64>(m_data.size()) - m_pos);
        if (m_failAt >= 0) {
            n = std::min(n, m_failAt - m_pos);
        }
        std::memcpy(out, m_data.constData() + m_pos, static_cast<std::size_t>(n));
        m_pos += n;
        return n;
    }
    qint64 writeData(const char*, qint64) override { return -1; }

private:
    QByteArray m_data;
    qint64 m_failAt;
    qint64 m_pos = 0;
};

// Accès direct (non séquentiel) dont la taille annoncée ment : comme un fournisseur content:// qui annonce
// 512 Mio et rend 8 octets.
class LyingDevice final : public QIODevice
{
public:
    LyingDevice(QByteArray data, qint64 announced) : m_data(std::move(data)), m_announced(announced) {}
    qint64 size() const override { return m_announced; }

protected:
    qint64 readData(char* out, qint64 max) override
    {
        const qint64 n = std::min(max, static_cast<qint64>(m_data.size()) - m_pos);
        std::memcpy(out, m_data.constData() + m_pos, static_cast<std::size_t>(n));
        m_pos += n;
        return n;
    }
    qint64 writeData(const char*, qint64) override { return -1; }

private:
    QByteArray m_data;
    qint64 m_announced;
    qint64 m_pos = 0;
};

} // namespace

// E/S par URL (décision 17) : fichier local par temporaire voisin puis renommage ; content:// aiguillé.
class TstFormatsUrlIo : public QObject
{
    Q_OBJECT

private slots:
    void writeThenReadLocalFile()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QUrl url = QUrl::fromLocalFile(dir.filePath(u"essai.actcraft"_s));
        QString error;
        QVERIFY2(formats::writeUrl(url, QByteArray("bonjour"), &error), qPrintable(error));
        const formats::UrlReadResult r = formats::readUrl(url);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.data, QByteArray("bonjour"));
    }

    void rewriteReplacesTheWholeFile()
    {
        QTemporaryDir dir;
        const QUrl url = QUrl::fromLocalFile(dir.filePath(u"essai.actcraft"_s));
        QVERIFY(formats::writeUrl(url, QByteArray(1000, 'a'), nullptr));
        QVERIFY(formats::writeUrl(url, QByteArray("court"), nullptr));
        QCOMPARE(formats::readUrl(url).data, QByteArray("court"));
        // Aucun temporaire laissé à côté.
        QCOMPARE(QDir(dir.path()).entryList(QDir::Files | QDir::Hidden).size(), 1);
    }

    void failedWriteKeepsThePreviousFile()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(u"essai.actcraft"_s);
        QVERIFY(formats::writeUrl(QUrl::fromLocalFile(path), QByteArray("ancien"), nullptr));
        QString error;
        // Dossier absent : rien n'est écrit, l'erreur dit pourquoi.
        QVERIFY(!formats::writeUrl(QUrl::fromLocalFile(dir.filePath(u"absent/x.actcraft"_s)), "y", &error));
        QVERIFY(!error.isEmpty());
        QCOMPARE(formats::readUrl(QUrl::fromLocalFile(path)).data, QByteArray("ancien"));
    }

    void existingFileIsNeverRewrittenDirectly()
    {
        // Un lecteur garde l'ancien fichier ouvert pendant l'enregistrement. Remplacement atomique : il lit
        // l'ancien contenu entier (POSIX : le renommage lui laisse l'ancien fichier ; Windows : le renommage
        // échoue tant qu'il est ouvert). Écriture en place (ouverture en écriture du fichier lui-même) : il
        // lirait le nouveau.
        QTemporaryDir dir;
        const QString path = dir.filePath(u"essai.actcraft"_s);
        const QByteArray before("ancien contenu, plus long");
        QVERIFY(formats::writeUrl(QUrl::fromLocalFile(path), before, nullptr));
        QFile reader(path);
        QVERIFY(reader.open(QIODevice::ReadOnly));
        QString error;
        const bool written = formats::writeUrl(QUrl::fromLocalFile(path), QByteArray("nouveau"), &error);
        QCOMPARE(reader.readAll(), before);
        reader.close();
        // Sur le disque : l'un ou l'autre en entier, et aucun temporaire laissé, même après un échec.
        const QByteArray now = formats::readUrl(QUrl::fromLocalFile(path)).data;
        QVERIFY2(written ? now == "nouveau" : (now == before && !error.isEmpty()), qPrintable(error));
        QCOMPARE(QDir(dir.path()).entryList(QDir::Files | QDir::Hidden).size(), 1);
    }

    void readRefusesMissingAndOversizedFiles()
    {
        QTemporaryDir dir;
        const formats::UrlReadResult missing =
            formats::readUrl(QUrl::fromLocalFile(dir.filePath(u"absent"_s)));
        QVERIFY(!missing.ok);
        QVERIFY(!missing.error.isEmpty());
        const QUrl big = QUrl::fromLocalFile(dir.filePath(u"gros"_s));
        QVERIFY(formats::writeUrl(big, QByteArray(100, 'x'), nullptr));
        const formats::UrlReadResult tooBig = formats::readUrl(big, 10);
        QVERIFY(!tooBig.ok);
        QVERIFY2(tooBig.error.contains(u"volumineux"_s), qPrintable(tooBig.error));
    }

    void sequentialDeviceOfUnknownSizeIsReadToTheEnd()
    {
        const QByteArray payload(5 * (1 << 20) / 2, 'q'); // 2,5 Mio : trois blocs
        SequentialDevice device(payload);
        QVERIFY(device.open(QIODevice::ReadOnly));
        QVERIFY(device.atEnd()); // ce que verrait une boucle « tant que pas atEnd() » : rien
        QString error;
        const std::optional<QByteArray> all = formats::readBounded(device, 1ll << 30, &error);
        QVERIFY2(all, qPrintable(error));
        QCOMPARE(all->size(), payload.size());
        SequentialDevice bounded(payload);
        QVERIFY(bounded.open(QIODevice::ReadOnly));
        QVERIFY(!formats::readBounded(bounded, 1 << 20, &error));
        QVERIFY(error.contains(u"volumineux"_s));
    }

    void readErrorIsNotTakenForTheEnd()
    {
        // 2 Mio lus puis une erreur : pas un fichier de 2 Mio.
        SequentialDevice device(QByteArray(3 << 20, 'q'), 2 << 20);
        QVERIFY(device.open(QIODevice::ReadOnly));
        QString error;
        QVERIFY(!formats::readBounded(device, 1ll << 30, &error));
        QVERIFY2(error.contains(u"lecture"_s), qPrintable(error));
    }

    void announcedSizeIsRefusedBeforeReading()
    {
        // Taille annoncée au-delà de la borne : refusé sans lire un octet.
        QByteArray bytes(100, 'x');
        QBuffer buffer(&bytes);
        QVERIFY(buffer.open(QIODevice::ReadOnly));
        QString error;
        QVERIFY(!formats::readBounded(buffer, 10, &error));
        QVERIFY2(error.contains(u"volumineux"_s), qPrintable(error));
        QCOMPARE(buffer.pos(), qint64{0});
    }

    void announcedSizeReservesAtMostAModestBlock()
    {
        // La taille annoncée d'un content:// vient du fournisseur : réservée jusqu'à 64 Mio, pas au-delà.
        LyingDevice device(QByteArray("8 octets"), 512ll << 20);
        QVERIFY(device.open(QIODevice::ReadOnly));
        QString error;
        const std::optional<QByteArray> data = formats::readBounded(device, 1ll << 30, &error);
        QVERIFY2(data, qPrintable(error));
        QCOMPARE(*data, QByteArray("8 octets"));
        QVERIFY2(data->capacity() <= (qsizetype{64} << 20), qPrintable(QString::number(data->capacity())));
    }

    void smallFileDoesNotReserveTheMaximum()
    {
        // Borne énorme : une lecture par file.read(maxBytes + 1) réserverait 1 Tio avant de lire, et
        // échouerait.
        QTemporaryDir dir;
        const QUrl url = QUrl::fromLocalFile(dir.filePath(u"petit"_s));
        QVERIFY(formats::writeUrl(url, QByteArray("7 octets"), nullptr));
        const formats::UrlReadResult r = formats::readUrl(url, 1ll << 40);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.data, QByteArray("7 octets"));
    }

    void unsupportedSchemeIsRefused()
    {
        QString error;
        QVERIFY(!formats::writeUrl(QUrl(u"https://exemple.org/x.actcraft"_s), "x", &error));
        QVERIFY2(error.contains(u"non prise en charge"_s), qPrintable(error));
        QVERIFY(!formats::readUrl(QUrl(u"https://exemple.org/x.actcraft"_s)).ok);
    }

    void argumentsBecomeUrls()
    {
#ifdef Q_OS_WIN
        const QUrl local = formats::urlFromArgument(u"C:/Users/x/doc.actcraft"_s);
        QVERIFY(local.isLocalFile());
        QCOMPARE(QDir::fromNativeSeparators(local.toLocalFile()), u"C:/Users/x/doc.actcraft"_s);
        QCOMPARE(formats::urlFromArgument(QUrl(u"file:///C:/a.actcraft"_s)).toLocalFile(),
                 u"C:/a.actcraft"_s);
#else
        const QUrl local = formats::urlFromArgument(u"/tmp/doc.actcraft"_s);
        QVERIFY(local.isLocalFile());
        QCOMPARE(local.toLocalFile(), u"/tmp/doc.actcraft"_s);
        QCOMPARE(formats::urlFromArgument(QUrl(u"file:///tmp/a.actcraft"_s)).toLocalFile(),
                 u"/tmp/a.actcraft"_s);
#endif
        const QUrl relative = formats::urlFromArgument(u"sous/doc.actcraft"_s);
        QCOMPARE(relative.toLocalFile(), QDir::current().absoluteFilePath(u"sous/doc.actcraft"_s));
        const QUrl content =
            formats::urlFromArgument(u"content://com.android.externalstorage.documents/document/x"_s);
        QCOMPARE(content.scheme(), u"content"_s);
        QVERIFY(formats::urlFromArgument(QVariant()).isEmpty());
        QVERIFY(formats::urlFromArgument(u""_s).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TstFormatsUrlIo)
#include "tst_formats_urlio.moc"
