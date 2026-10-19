#include <QtTest>

class TstCoreEquation : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L7-S8 : rempli par L7-T24."); }
};

QTEST_GUILESS_MAIN(TstCoreEquation)
#include "tst_core_equation.moc"
