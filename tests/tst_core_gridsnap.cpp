#include <QtTest>

class TstCoreGridsnap : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L4-T1 : rempli par L4-T5."); }
};

QTEST_GUILESS_MAIN(TstCoreGridsnap)
#include "tst_core_gridsnap.moc"
