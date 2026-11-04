#include <QtTest>

class TstActionsProfile : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T4."); }
};

QTEST_MAIN(TstActionsProfile)
#include "tst_actions_profile.moc"
