#include <QtTest>

class TstCoreRestrictors : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T15."); }
};

QTEST_MAIN(TstCoreRestrictors)
#include "tst_core_restrictors.moc"
