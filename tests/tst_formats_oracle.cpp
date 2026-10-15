#include <actcraft/formats/flipchart.h>
#include <actcraft/formats/flipchartdump.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTest>

using namespace actcraft::formats::flipchart;
using namespace Qt::StringLiterals;

// Oracle local (décision 19) : chaque paperboard de samples/, lu en place, a le même export canonique que le
// lecteur Python. Sauté sans ACTCRAFT_SAMPLES_DIR (CI, poste sans l'ancien dépôt).
class TstFormatsOracle : public QObject
{
    Q_OBJECT

public:
    bool skipped = false; // lu par main : code de sortie 77

private slots:
    void everySampleMatchesThePythonReader()
    {
        const QString samples = qEnvironmentVariable("ACTCRAFT_SAMPLES_DIR");
        if (samples.isEmpty()) {
            skipped = true;
            QSKIP("ACTCRAFT_SAMPLES_DIR absent : oracle local seulement (décision 19, DA4)");
        }
        QString oracle = qEnvironmentVariable("ACTCRAFT_ORACLE_DIR");
        if (oracle.isEmpty()) {
            oracle = QDir(samples).absoluteFilePath(u"../build/oracle-flipchart"_s);
        }
        QVERIFY2(
            QDir(oracle).exists(),
            qPrintable(u"JSON de l'oracle absents : %1 (lancer tools/oracle_flipchart.py)"_s.arg(oracle)));
        const QFileInfoList files = QDir(samples).entryInfoList({u"*.flipchart"_s}, QDir::Files, QDir::Name);
        QVERIFY(!files.isEmpty());
        // Fraîcheur : un JSON par paperboard, aucun en trop, aucun plus ancien que le script qui l'a écrit.
        const QFileInfoList written = QDir(oracle).entryInfoList({u"*.json"_s}, QDir::Files, QDir::Name);
        QVERIFY2(written.size() == files.size(),
                 qPrintable(u"%1 JSON pour %2 paperboards"_s.arg(written.size()).arg(files.size())));
        const QFileInfo script(QDir(samples).absoluteFilePath(u"../tools/oracle_flipchart.py"_s));
        int pages = 0;
        QStringList failures;
        for (const QFileInfo& info : files) {
            QFile file(info.absoluteFilePath());
            QVERIFY(file.open(QIODevice::ReadOnly)); // lecture seule : samples/ n'est jamais modifié
            const QByteArray bytes = file.readAll();
            const FlipchartReadResult r = readFlipchart(bytes);
            if (!r.ok) {
                failures << u"%1 : %2"_s.arg(info.fileName(), r.error);
                continue;
            }
            pages += static_cast<int>(r.document.pageCount());
            QFile expectedFile(QDir(oracle).filePath(info.completeBaseName() + u".json"_s));
            if (!expectedFile.open(QIODevice::ReadOnly)) {
                failures << u"%1 : JSON de l'oracle absent"_s.arg(info.fileName());
                continue;
            }
            if (script.exists() && QFileInfo(expectedFile).lastModified() < script.lastModified()) {
                failures << u"%1 : JSON plus ancien que le script de l'oracle"_s.arg(info.fileName());
                continue;
            }
            QJsonParseError parse{};
            const QJsonDocument parsed = QJsonDocument::fromJson(expectedFile.readAll(), &parse);
            if (parse.error != QJsonParseError::NoError || !parsed.isObject()) {
                failures << u"%1 : JSON de l'oracle illisible (%2)"_s.arg(info.fileName(),
                                                                          parse.errorString());
                continue;
            }
            QJsonObject expected = parsed.object();
            const QString source = expected.take(u"source"_s).toString();
            if (source != QString::fromStdString(actcraft::core::sha256Hex(bytes))) {
                failures << u"%1 : JSON de l'oracle écrit pour un autre paperboard (empreinte)"_s.arg(
                    info.fileName());
                continue;
            }
            for (const QString& diff : compareCanonical(expected, canonicalDump(r))) {
                failures << u"%1 : %2"_s.arg(info.fileName(), diff);
            }
        }
        for (const QString& f : failures.mid(0, 60)) {
            qWarning("ECART %s", qPrintable(f));
        }
        QVERIFY2(failures.isEmpty(), qPrintable(u"%1 écart(s)"_s.arg(failures.size())));
        qInfo("ORACLE OK : %d paperboards, %d pages, 0 écart", static_cast<int>(files.size()), pages);
    }
};

// Sauté : code de sortie 77, que ctest compte « Skipped » (SKIP_RETURN_CODE, tests/flipchart.cmake) ; QSKIP
// seul sortirait 0, compté « Passed ».
int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    TstFormatsOracle test;
    const int failures = QTest::qExec(&test, argc, argv);
    return failures == 0 && test.skipped ? 77 : failures;
}
#include "tst_formats_oracle.moc"
