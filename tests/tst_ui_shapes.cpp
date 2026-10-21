#include <QtTest>

class TstUiShapes : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L4-T1 : rempli par L4-T11."); }
};

QTEST_MAIN(TstUiShapes)
#include "tst_ui_shapes.moc"
