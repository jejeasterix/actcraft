#include <QtTest>

class TstActionsLockable : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T3."); }
};

QTEST_MAIN(TstActionsLockable)
#include "tst_actions_lockable.moc"
