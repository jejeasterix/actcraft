#include <QTest>

class TstUiLift : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L0-S : rempli par L3-T12."); }
};

QTEST_MAIN(TstUiLift)
#include "tst_ui_lift.moc"
