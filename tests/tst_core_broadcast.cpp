#include <QtTest>

class TstCoreBroadcast : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T31."); }
};

QTEST_MAIN(TstCoreBroadcast)
#include "tst_core_broadcast.moc"
