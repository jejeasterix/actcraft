#include <QtTest>

class TstCoreContainers : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T14."); }
};

QTEST_MAIN(TstCoreContainers)
#include "tst_core_containers.moc"
