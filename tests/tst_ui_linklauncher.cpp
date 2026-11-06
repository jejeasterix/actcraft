#include <QtTest>

class TstUiLinklauncher : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T10."); }
};

QTEST_MAIN(TstUiLinklauncher)
#include "tst_ui_linklauncher.moc"
