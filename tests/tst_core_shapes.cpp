#include <QtTest>

class TstCoreShapes : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L4-T1 : rempli par L4-T3."); }
};

QTEST_GUILESS_MAIN(TstCoreShapes)
#include "tst_core_shapes.moc"
