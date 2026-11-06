#include <QtTest>

class TstUiActionrunner : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T9."); }
};

QTEST_MAIN(TstUiActionrunner)
#include "tst_ui_actionrunner.moc"
