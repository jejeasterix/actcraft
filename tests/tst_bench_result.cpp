#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>
#include <QTest>

// Juge le JSON produit par tst_bench_smoke (fixture ctest bench_smoke_json).
class TstBenchResult : public QObject
{
    Q_OBJECT

private:
    QJsonObject m_root;
    QByteArray m_text;

private slots:
    void initTestCase()
    {
        const QString path = qEnvironmentVariable("ACTCRAFT_BENCH_SMOKE_JSON");
        QVERIFY2(!path.isEmpty(), "ACTCRAFT_BENCH_SMOKE_JSON absent");
        QFile file(path);
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(path));
        m_text = file.readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(m_text);
        QVERIFY(doc.isObject());
        m_root = doc.object();
    }

    void formatIsVersioned()
    {
        QCOMPARE(m_root.value("format").toString(), QStringLiteral("actcraft-bench/1"));
    }

    void softwareRenderer()
    {
        QCOMPARE(m_root.value("graphics_api").toString(), QStringLiteral("software"));
        QVERIFY(m_root.value("informative").toBool());
    }

    void validityFollowsBuildType()
    {
        QCOMPARE(m_root.value("valid_for_measurement").toBool(), ACTCRAFT_EXPECT_VALID != 0);
        if constexpr (ACTCRAFT_EXPECT_VALID == 0) {
            const QJsonArray reasons = m_root.value("verdict").toObject().value("invalid_reasons").toArray();
            QVERIFY(reasons.contains(QStringLiteral("build-not-release")));
        }
    }

    void threeScenariosWithFrames()
    {
        const QJsonArray scenarios = m_root.value("scenarios").toArray();
        QCOMPARE(scenarios.size(), 3);
        const QStringList names{QStringLiteral("stroke"), QStringLiteral("zoom"), QStringLiteral("scroll")};
        for (qsizetype i = 0; i < 3; ++i) {
            const QJsonObject s = scenarios.at(i).toObject();
            QCOMPARE(s.value("name").toString(), names.at(i));
            QVERIFY(s.value("frames").toObject().value("count").toInt() > 0);
            QVERIFY(s.value("frames").toObject().value("active").toInt() > 0);
        }
    }

    void strokeHasLatency()
    {
        const QJsonObject latency =
            m_root.value("scenarios").toArray().at(0).toObject().value("latency").toObject();
        QVERIFY(latency.value("samples").toInt() > 0);
        QVERIFY(latency.value("total_ms").toObject().value("p50").toDouble() > 0.0);
    }

    void contentCheckPasses()
    {
        const QJsonObject check = m_root.value("content_check").toObject();
        QVERIFY(check.value("ok").toBool());
        const QJsonArray checks = check.value("checks").toArray();
        QCOMPARE(checks.size(), 4);
        for (const QJsonValue& c : checks) {
            QVERIFY2(c.toObject().value("ok").toBool(), qPrintable(c.toObject().value("name").toString()));
        }
    }

    void verdictHasCriteria()
    {
        QVERIFY(!m_root.value("verdict").toObject().value("criteria").toArray().isEmpty());
    }

    void noHostName()
    {
        const QString host = QSysInfo::machineHostName();
        if (host.size() >= 3) {
            QVERIFY(!QString::fromUtf8(m_text).contains(host, Qt::CaseInsensitive));
        }
    }
};

QTEST_GUILESS_MAIN(TstBenchResult)
#include "tst_bench_result.moc"
