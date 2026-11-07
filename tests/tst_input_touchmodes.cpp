#include <QtTest>

class TstInputTouchmodes : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T4."); }
};

QTEST_GUILESS_MAIN(TstInputTouchmodes)
#include "tst_input_touchmodes.moc"
