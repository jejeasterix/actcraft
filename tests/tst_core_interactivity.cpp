#include <QtTest>

class TstCoreInteractivity : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T3."); }
};

QTEST_MAIN(TstCoreInteractivity)
#include "tst_core_interactivity.moc"
