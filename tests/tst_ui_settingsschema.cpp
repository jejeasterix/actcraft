#include <QtTest>

class TstUiSettingsschema : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T27."); }
};

QTEST_MAIN(TstUiSettingsschema)
#include "tst_ui_settingsschema.moc"
