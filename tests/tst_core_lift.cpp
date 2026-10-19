#include <QTest>

class TstCoreLift : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L0-S : rempli par L3-T4."); }
};

QTEST_GUILESS_MAIN(TstCoreLift)
#include "tst_core_lift.moc"
