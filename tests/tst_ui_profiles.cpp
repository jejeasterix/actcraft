#include <QtTest>

class TstUiProfiles : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T6."); }
};

QTEST_MAIN(TstUiProfiles)
#include "tst_ui_profiles.moc"
