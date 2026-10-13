#include <actcraft/bench/machineinfo.h>
#include <actcraft/bench/result.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>
#include <QTemporaryDir>
#include <QTest>

using namespace actcraft::bench;

namespace {

constexpr std::int64_t ms(double v)
{
    return static_cast<std::int64_t>(v * 1'000'000.0);
}

BenchResult sample()
{
    BenchResult r;
    r.startedAt = QDateTime(QDate(2026, 10, 13), QTime(14, 3, 22));
    r.machine = readMachineInfo();
    r.machine.model = QStringLiteral("Dell Inc. Inspiron 15 3510");
    r.graphicsApi = QStringLiteral("direct3d11");
    r.cache = true;
    r.scenarioSet = QStringLiteral("court");
    ScenarioResult stroke;
    stroke.name = QStringLiteral("stroke");
    stroke.startNs = ms(1);
    stroke.frames.swapNs = {ms(1), ms(17), ms(34)};
    stroke.frames.inputNs = {ms(5), ms(20)};
    stroke.frames.refreshPeriodNs = ms(16.67);
    stroke.intervals = classifyIntervals(stroke.frames);
    stroke.latency = computeLatency({ms(2), ms(7)}, {{1, ms(10), ms(17)}});
    stroke.memory.privatePeak = 1000;
    r.scenarios.push_back(stroke);
    ScenarioResult zoom;
    zoom.name = QStringLiteral("zoom");
    r.scenarios.push_back(zoom);
    return r;
}

} // namespace

class TstBenchJson : public QObject
{
    Q_OBJECT

private slots:
    void fileNameHasDateModelAndApi()
    {
        QCOMPARE(resultFileName(sample()),
                 QStringLiteral("2026-10-13_140322-Dell-Inc-Inspiron-15-3510-direct3d11.json"));
    }

    void sanitizeKeepsSafeCharacters()
    {
        QCOMPARE(sanitizeForFileName(QStringLiteral("Mac Book/Air (M1)")), QStringLiteral("Mac-Book-Air-M1"));
        QCOMPARE(sanitizeForFileName(QString()), QStringLiteral("inconnu"));
    }

    void topLevelKeys()
    {
        const QJsonObject o = toJson(sample());
        QCOMPARE(o.value("format").toString(), QStringLiteral("actcraft-bench/1"));
        QCOMPARE(o.value("graphics_api").toString(), QStringLiteral("direct3d11"));
        QCOMPARE(o.value("cache").toBool(), true);
        QVERIFY(o.contains("valid_for_measurement"));
        QVERIFY(o.contains("within_budget"));
        QVERIFY(o.value("meta").toObject().contains("compiler_flags"));
        QVERIFY(o.value("meta").toObject().contains("limits"));
        QCOMPARE(o.value("content_check").toObject().value("ok").toBool(), false); // aucun contrôle
        QVERIFY(o.value("verdict").isObject());
    }

    void scenariosAndRawSeries()
    {
        const QJsonArray scenarios = toJson(sample()).value("scenarios").toArray();
        QCOMPARE(scenarios.size(), 2);
        const QJsonObject stroke = scenarios.at(0).toObject();
        QCOMPARE(stroke.value("name").toString(), QStringLiteral("stroke"));
        QCOMPARE(stroke.value("frames").toObject().value("count").toInt(), 3);
        QCOMPARE(stroke.value("raw").toObject().value("swap_ns").toArray().at(0).toDouble(), 0.0);
        QCOMPARE(stroke.value("raw").toObject().value("interval_class").toString(), QStringLiteral("AA"));
        QCOMPARE(stroke.value("latency").toObject().value("samples").toInt(), 2);
        QVERIFY(stroke.value("memory").toObject().value("gpu_peak_bytes").isNull());
        QVERIFY(scenarios.at(1).toObject().value("latency").isNull());
    }

    // Écart au plan : champs ajoutés par T13 (reprises, figements non mesurés, période invalide).
    void framesExposeResumeAndInvalidPeriod()
    {
        const QJsonArray scenarios = toJson(sample()).value("scenarios").toArray();
        const QJsonObject good = scenarios.at(0).toObject().value("frames").toObject();
        for (const char* key : {"resume", "resume_max_ms", "resume_over_limit", "freeze_unmeasured",
                                "invalid_refresh_period"}) {
            QVERIFY2(good.contains(QLatin1String(key)), key);
        }
        QCOMPARE(good.value("invalid_refresh_period").toBool(), false);
        // Sans période d'écran mesurée : mesure invalide, dite telle quelle (aucune valeur plausible
        // inventée).
        const QJsonObject bad = scenarios.at(1).toObject().value("frames").toObject();
        QCOMPARE(bad.value("invalid_refresh_period").toBool(), true);
        QCOMPARE(bad.value("refresh_period_ms").toDouble(), 0.0);
    }

    void noHostOrUserName()
    {
        const QString text = QString::fromUtf8(QJsonDocument(toJson(sample())).toJson());
        const QStringList secrets{QSysInfo::machineHostName(), qEnvironmentVariable("COMPUTERNAME"),
                                  qEnvironmentVariable("USERNAME"), qEnvironmentVariable("USER")};
        for (const QString& secret : secrets) {
            if (secret.size() >= 3) {
                QVERIFY2(!text.contains(secret, Qt::CaseInsensitive), qPrintable(secret));
            }
        }
    }

    void writeCreatesFolders()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("a/b/resultat.json"));
        QString error;
        QVERIFY2(writeResult(sample(), path, error), qPrintable(error));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(QJsonDocument::fromJson(file.readAll()).object().value("format").toString(),
                 QStringLiteral("actcraft-bench/1"));
    }

    void apiNames()
    {
        QCOMPARE(apiName(QSGRendererInterface::Software), QStringLiteral("software"));
        QCOMPARE(apiName(QSGRendererInterface::Direct3D11), QStringLiteral("direct3d11"));
        QCOMPARE(apiName(QSGRendererInterface::Metal), QStringLiteral("metal"));
    }
};

QTEST_GUILESS_MAIN(TstBenchJson)
#include "tst_bench_json.moc"
