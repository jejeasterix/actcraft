#include <QtTest>

class TstCorePagesettings : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L4-T1 : rempli par L4-T4."); }
};

QTEST_GUILESS_MAIN(TstCorePagesettings)
#include "tst_core_pagesettings.moc"
