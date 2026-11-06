#include <QtTest>

class TstActionsObjectactions : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T6."); }
};

QTEST_MAIN(TstActionsObjectactions)
#include "tst_actions_objectactions.moc"
