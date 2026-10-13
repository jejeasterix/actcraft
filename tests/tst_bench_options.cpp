#include <actcraft/bench/options.h>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
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
        QVERIFY(parseBenchOptions({"b", "--taille", "plein-ecran"}, o, error));
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

    void repeatedOptionIsRejected()
    {
        // app/renderbackend.cpp garde la première occurrence de --rendu : le banc ne doit pas en lire une
        // autre.
        const QList<QStringList> cases{
            {"b", "--rendu", "d3d11", "--rendu", "logiciel"},
            {"b", "--taille", "800x600", "--taille", "plein-ecran"},
            {"b", "--sans-cache", "--sans-cache"},
        };
        for (const QStringList& args : cases) {
            BenchOptions o;
            QString error;
            QVERIFY2(!parseBenchOptions(args, o, error), qPrintable(args.join(' ')));
            QVERIFY2(error.contains(QLatin1String("plusieurs fois")), qPrintable(error));
        }
    }

    void outputWithForeignExtensionIsRejected()
    {
        const QStringList bad{"x.txt", "bench-smoke.json.autre", "dossier.v2/resultat.csv"};
        for (const QString& path : bad) {
            BenchOptions o;
            QString error;
            QVERIFY2(!parseBenchOptions({"b", "--sortie", path}, o, error), qPrintable(path));
            QVERIFY2(error.contains(QLatin1String("--sortie")), qPrintable(error));
        }
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QFile plain(dir.filePath(QStringLiteral("fichier")));
        QVERIFY(plain.open(QIODevice::WriteOnly));
        plain.close();
        BenchOptions o;
        QString error;
        QVERIFY(!parseBenchOptions({"b", "--sortie", plain.fileName()}, o, error));
        QVERIFY(!error.isEmpty());
    }

    void outputAcceptedForms()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        for (const QString& path :
             {QStringLiteral("x.json"), QStringLiteral("X.JSON"), QStringLiteral("bench/results/"),
              QStringLiteral("resultats"), dir.path()}) {
            BenchOptions o;
            QString error;
            QVERIFY2(parseBenchOptions({"b", "--sortie", path}, o, error), qPrintable(path + error));
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
