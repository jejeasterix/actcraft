#include <QTest>

class TstInputRouting : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L0-S : rempli par L3-T5."); }
};

QTEST_GUILESS_MAIN(TstInputRouting)
#include "tst_input_routing.moc"
