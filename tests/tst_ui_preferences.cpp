#include <QtTest>

class TstUiPreferences : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T5."); }
};

QTEST_MAIN(TstUiPreferences)
#include "tst_ui_preferences.moc"
