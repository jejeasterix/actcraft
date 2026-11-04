#include <QtTest>

class TstUiHelp : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T35."); }
};

QTEST_MAIN(TstUiHelp)
#include "tst_ui_help.moc"
