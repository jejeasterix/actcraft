#include <QtTest>

class TstBenchResultsdir : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T15."); }
};

QTEST_GUILESS_MAIN(TstBenchResultsdir)
#include "tst_bench_resultsdir.moc"
