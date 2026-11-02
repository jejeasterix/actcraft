#include <QTest>

// Test réservé par L5-T1 : le contenu vient de L5-T14.
class TestReserved : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L5-T14 : test réservé par L5-T1, rempli par L5-T14"); }
};

QTEST_MAIN(TestReserved)
#include "tst_ui_objectediting.moc"
