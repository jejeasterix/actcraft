#include <QtTest>

class TstToolsTwousers : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T8."); }
};

QTEST_GUILESS_MAIN(TstToolsTwousers)
#include "tst_tools_twousers.moc"
