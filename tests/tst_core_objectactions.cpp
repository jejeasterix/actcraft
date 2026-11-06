#include <QtTest>

class TstCoreObjectactions : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T7."); }
};

QTEST_MAIN(TstCoreObjectactions)
#include "tst_core_objectactions.moc"
