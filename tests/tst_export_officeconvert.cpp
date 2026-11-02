#include <QTest>

// Test réservé par L5-T1 : le contenu vient de L5-T29.
class TestReserved : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L5-T29 : test réservé par L5-T1, rempli par L5-T29"); }
};

QTEST_GUILESS_MAIN(TestReserved)
#include "tst_export_officeconvert.moc"
