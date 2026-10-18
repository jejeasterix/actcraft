#include <QTest>

class TstCoreErase : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L0-S : rempli par L3-T3."); }
};

QTEST_GUILESS_MAIN(TstCoreErase)
#include "tst_core_erase.moc"
