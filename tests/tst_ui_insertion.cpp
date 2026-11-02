#include <QTest>

// Test réservé par L5-T1 : le contenu vient de L5-T15.
class TestReserved : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L5-T15 : test réservé par L5-T1, rempli par L5-T15"); }
};

QTEST_MAIN(TestReserved)
#include "tst_ui_insertion.moc"
