#include <QtTest>

class TstCoreZones : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L4-T1 : rempli par L4-T27."); }
};

QTEST_GUILESS_MAIN(TstCoreZones)
#include "tst_core_zones.moc"
