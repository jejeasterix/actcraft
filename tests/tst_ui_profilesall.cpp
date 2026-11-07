#include <QtTest>

class TstUiProfilesall : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T27."); }
};

QTEST_MAIN(TstUiProfilesall)
#include "tst_ui_profilesall.moc"
