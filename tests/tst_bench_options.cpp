#include <actcraft/bench/options.h>

#include <QDir>
#include <QTest>

using namespace actcraft::bench;

class TstBenchOptions : public QObject
{
    Q_OBJECT

private slots:
    void defaults()
    {
        BenchOptions o;
        QString error;
        QVERIFY(parseBenchOptions({QStringLiteral("actcraft-bench")}, o, error));
        QCOMPARE(o.scenarioSet, ScenarioSet::Complet);
        QVERIFY(o.cacheEnabled);
        QVERIFY(o.fullScreen);
        QVERIFY(o.output.isEmpty());
        QVERIFY(o.capturesDir.isEmpty());
#ifdef Q_OS_WIN
        QCOMPARE(o.requestedBackend, QStringLiteral("d3d11"));
#else
        QCOMPARE(o.requestedBackend, QStringLiteral("auto"));
#endif
    }

    void everyOption()
    {
        BenchOptions o;
        QString error;
        QVERIFY(parseBenchOptions({"actcraft-bench", "--rendu", "logiciel", "--scenario", "court", "--sortie",
                                   "x.json", "--sans-cache", "--taille", "1280x800", "--captures", "cap"},
                                  o, error));
        QCOMPARE(o.requestedBackend, QStringLiteral("logiciel"));
        QCOMPARE(o.scenarioSet, ScenarioSet::Court);
        QCOMPARE(o.output, QStringLiteral("x.json"));
        QVERIFY(!o.cacheEnabled);
        QVERIFY(!o.fullScreen);
        QCOMPARE(o.windowSize, QSize(1280, 800));
        QCOMPARE(o.capturesDir, QStringLiteral("cap"));
    }

    void fullScreenCanBeAskedExplicitly()
    {
        BenchOptions o;
        QString error;
        QVERIFY(parseBenchOptions({"b", "--taille", "800x600", "--taille", "plein-ecran"}, o, error));
        QVERIFY(o.fullScreen);
    }

    void badValuesAreRejected()
    {
        const QList<QStringList> cases{
            {"b", "--taille", "100x100"}, {"b", "--taille", "grand"},
            {"b", "--scenario", "long"},  {"b", "--sortie"},
            {"b", "--inconnue"},          {"b", "--rendu"},
        };
        for (const QStringList& args : cases) {
            BenchOptions o;
            QString error;
            QVERIFY2(!parseBenchOptions(args, o, error), qPrintable(args.join(' ')));
            QVERIFY(!error.isEmpty());
        }
    }

    void outputPath()
    {
        BenchOptions o;
        QCOMPARE(resolveOutputPath(o, QStringLiteral("D:/r"), QStringLiteral("f.json")),
                 QDir(QStringLiteral("D:/r")).filePath(QStringLiteral("f.json")));
        o.output = QStringLiteral("bench/results/");
        QCOMPARE(resolveOutputPath(o, QStringLiteral("D:/r"), QStringLiteral("f.json")),
                 QStringLiteral("bench/results/f.json"));
        o.output = QStringLiteral("sortie.json");
        QCOMPARE(resolveOutputPath(o, QStringLiteral("D:/r"), QStringLiteral("f.json")),
                 QStringLiteral("sortie.json"));
    }
};

QTEST_GUILESS_MAIN(TstBenchOptions)
#include "tst_bench_options.moc"
