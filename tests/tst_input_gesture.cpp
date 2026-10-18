#include <QTest>

class TstInputGesture : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L0-S : rempli par L3-T5."); }
};

QTEST_GUILESS_MAIN(TstInputGesture)
#include "tst_input_gesture.moc"
