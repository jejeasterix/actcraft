#include <actcraft/bench/options.h>

#include <QtTest>

using namespace actcraft::bench;

// Dossier des résultats du banc selon la plateforme, et options reçues en une seule chaîne (Android).
class TstBenchResultsdir : public QObject
{
    Q_OBJECT
private slots:
    void desktopDefaultUnchanged()
    {
        QCOMPARE(resultsDirFor(Platform::Desktop, QStringLiteral("/opt/actcraft")),
                 QStringLiteral("/opt/actcraft/resultats-banc"));
    }

    void androidDefaultIsExternalAppFiles()
    {
        const QString files =
            QStringLiteral("/sdcard/Android/data/io.github.jejeasterix.actcraft.banc/files");
        QCOMPARE(resultsDirFor(Platform::Android, files), files + QStringLiteral("/bench-results"));
    }

    void argumentsParsedFromSingleString()
    {
        const QStringList args = {QStringLiteral("actcraft-bench"),
                                  QStringLiteral("--scenario court --taille 1280x800")};
        const QStringList expanded = expandSingleStringArguments(args);
        QCOMPARE(expanded.size(), 5);
        // Des arguments déjà séparés restent tels quels.
        QCOMPARE(expandSingleStringArguments(expanded), expanded);

        BenchOptions options;
        QString error;
        QVERIFY2(parseBenchOptions(expanded, options, error), qPrintable(error));
        QCOMPARE(options.scenarioSet, ScenarioSet::Court);
        QVERIFY(!options.fullScreen);
        QCOMPARE(options.windowSize, QSize(1280, 800));
    }

    void explicitOutputWins()
    {
        const QString files =
            QStringLiteral("/sdcard/Android/data/io.github.jejeasterix.actcraft.banc/files");
        const QString dir = resultsDirFor(Platform::Android, files);
        BenchOptions options;
        QString error;
        QVERIFY(parseBenchOptions({QStringLiteral("b"), QStringLiteral("--sortie"), QStringLiteral("/tmp/x")},
                                  options, error));
        QCOMPARE(resolveOutputPath(options, dir, QStringLiteral("r.json")), QStringLiteral("/tmp/x/r.json"));
        QVERIFY(parseBenchOptions({QStringLiteral("b")}, options, error));
        QCOMPARE(resolveOutputPath(options, dir, QStringLiteral("r.json")), dir + QStringLiteral("/r.json"));
    }
};

QTEST_GUILESS_MAIN(TstBenchResultsdir)
#include "tst_bench_resultsdir.moc"
