#include <QTest>

class TstCoreReplace : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L0-S : rempli par L0-S3."); }
};

QTEST_GUILESS_MAIN(TstCoreReplace)
#include "tst_core_replace.moc"
