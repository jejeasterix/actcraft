#include <QTest>

class TstCoreObjectedits : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L0-S : rempli par L3-T4."); }
};

QTEST_GUILESS_MAIN(TstCoreObjectedits)
#include "tst_core_objectedits.moc"
