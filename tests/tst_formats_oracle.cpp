#include <actcraft/formats/flipchart.h>
#include <actcraft/formats/flipchartdump.h>

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

private slots:
    void everySampleMatchesThePythonReader()
    {
        const QString samples = qEnvironmentVariable("ACTCRAFT_SAMPLES_DIR");
        if (samples.isEmpty()) {
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
        int pages = 0;
        QStringList failures;
        for (const QFileInfo& info : files) {
            QFile file(info.absoluteFilePath());
            QVERIFY(file.open(QIODevice::ReadOnly)); // lecture seule : samples/ n'est jamais modifié
            const FlipchartReadResult r = readFlipchart(file.readAll());
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
            QJsonParseError parse{};
            const QJsonDocument expected = QJsonDocument::fromJson(expectedFile.readAll(), &parse);
            if (parse.error != QJsonParseError::NoError || !expected.isObject()) {
                failures << u"%1 : JSON de l'oracle illisible (%2)"_s.arg(info.fileName(),
                                                                          parse.errorString());
                continue;
            }
            for (const QString& diff : compareCanonical(expected.object(), canonicalDump(r.document))) {
                failures << u"%1 : %2"_s.arg(info.fileName(), diff);
            }
        }
        for (const QString& f : failures.mid(0, 40)) {
            qWarning("ECART %s", qPrintable(f));
        }
        QVERIFY2(failures.isEmpty(), qPrintable(u"%1 écart(s)"_s.arg(failures.size())));
        qInfo("ORACLE OK : %d paperboards, %d pages, 0 écart", static_cast<int>(files.size()), pages);
    }
};

QTEST_GUILESS_MAIN(TstFormatsOracle)
#include "tst_formats_oracle.moc"
