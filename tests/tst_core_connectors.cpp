#include <QTest>

// Test réservé par L5-T1 : le contenu vient de L5-T10.
class TestReserved : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L5-T10 : test réservé par L5-T1, rempli par L5-T10"); }
};

QTEST_GUILESS_MAIN(TestReserved)
#include "tst_core_connectors.moc"
