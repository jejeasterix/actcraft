#include <QtTest>

class TstInputTwousers : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T7."); }
};

QTEST_MAIN(TstInputTwousers)
#include "tst_input_twousers.moc"
