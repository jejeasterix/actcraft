#include <QtTest>

class TstUiLockall : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T26."); }
};

QTEST_MAIN(TstUiLockall)
#include "tst_ui_lockall.moc"
