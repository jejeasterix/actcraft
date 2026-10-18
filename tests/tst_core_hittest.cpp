#include <QTest>

class TstCoreHittest : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L0-S : rempli par L3-T3."); }
};

QTEST_GUILESS_MAIN(TstCoreHittest)
#include "tst_core_hittest.moc"
