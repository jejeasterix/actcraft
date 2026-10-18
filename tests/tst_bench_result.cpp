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

    // L2-T23 : « page » ; L10-T1 : « pauses » (reprises après le délai d'inactivité du cache).
    void fiveScenariosWithFrames()
    {
        const QJsonArray scenarios = m_root.value("scenarios").toArray();
        QCOMPARE(scenarios.size(), 5);
        const QStringList names{QStringLiteral("stroke"), QStringLiteral("zoom"), QStringLiteral("scroll"),
                                QStringLiteral("page"), QStringLiteral("pauses")};
        for (qsizetype i = 0; i < 5; ++i) {
            const QJsonObject s = scenarios.at(i).toObject();
            QCOMPARE(s.value("name").toString(), names.at(i));
            QVERIFY(s.value("frames").toObject().value("count").toInt() > 0);
            QVERIFY(s.value("frames").toObject().value("active").toInt() > 0);
        }
    }

    // L10-T1 : instants des demandes de redessin du cache et de leurs fins, par scénario. Implémentation du
    // lot 2 : au moins un redessin par trait de « pauses » (4 en court).
    void cacheRefreshesAreRecorded()
    {
        const QJsonObject pauses = m_root.value("scenarios").toArray().at(4).toObject();
        const QJsonObject cache = pauses.value("cache").toObject();
        QVERIFY(cache.contains("refreshes"));
        const QJsonObject raw = pauses.value("raw").toObject();
        const QJsonArray requests = raw.value("cache_request_ns").toArray();
        const QJsonArray updates = raw.value("cache_updated_ns").toArray();
        // L10-T4 : versement différé. En court, 4 traits ; la reprise k = 0 tombe à 500 ms tout juste et
        // relance la minuterie, la reprise k = 1 (524 ms) fait verser. En debug, ce redessin du cache en
        // rendu logiciel (environ 300 ms, fumée du 2026-10-18) retarde le relâché du trait suivant et ramène
        // la pause k = 2 (548 ms prévus) sous le délai : seul ce premier versement est garanti. Le garde du
        // différé, sans minutage, est deferredFoldShowsInTheResult.
        QVERIFY2(cache.value("refreshes").toInt() >= 1, "au moins un versement attendu dans pauses");
        QVERIFY(requests.size() >= 1);
        QVERIFY(updates.size() >= 1);
        for (qsizetype i = 1; i < requests.size(); ++i) {
            QVERIFY(requests.at(i).toDouble() >= requests.at(i - 1).toDouble());
        }
    }

    // L10-T4 : aucun redessin du cache pendant « stroke » (pauses de 60 ms en court, sous le délai
    // d'inactivité), et au moins un objet a attendu au-dessus du cache dans « pauses ».
    void deferredFoldShowsInTheResult()
    {
        const QJsonArray scenarios = m_root.value("scenarios").toArray();
        QCOMPARE(scenarios.at(0).toObject().value("cache").toObject().value("refreshes").toInt(), 0);
        QVERIFY(scenarios.at(4).toObject().value("cache").toObject().value("pending_max").toInt() >= 1);
    }

    // 2e revue L2-T23 : le câblage du runner (marques, prêts relevés, fenêtres) classe actifs les intervalles
    // des changements, et eux seuls. Sans les marques, seul l'intervalle de chaque commande serait actif (au
    // plus 7) ; sans prêt relevé, tout serait actif de la première commande à la fin. Il faut donc un
    // intervalle au repos après la première commande : un changement au moins (le zoom posé a 700 ms) finit
    // avant la commande suivante, même en debug sous charge (relevé : 1 sur 12 intervalles, 111 s de banc).
    void pageChangesAreActiveUntilShown()
    {
        const QJsonObject page = m_root.value("scenarios").toArray().at(3).toObject();
        const QJsonObject raw = page.value("raw").toObject();
        const QJsonArray inputs = raw.value("input_ns").toArray();
        const QJsonArray swaps = raw.value("swap_ns").toArray();
        const QString classes = raw.value("interval_class").toString();
        QCOMPARE(inputs.size(), 7);
        QCOMPARE(classes.size(), swaps.size() - 1);
        QVERIFY(page.value("frames").toObject().value("active").toInt() > inputs.size());
        const double first = inputs.first().toDouble();
        qsizetype idleAfter = 0;
        for (qsizetype k = 0; k < classes.size(); ++k) {
            if (swaps.at(k).toDouble() >= first && classes.at(k) == u'I') {
                ++idleAfter;
            }
        }
        QVERIFY(idleAfter > 0);
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
        QCOMPARE(checks.size(), 5); // L10-T4 : stroke-folded
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
