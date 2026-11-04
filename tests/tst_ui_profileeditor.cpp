#include <QtTest>

class TstUiProfileeditor : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T25."); }
};

QTEST_MAIN(TstUiProfileeditor)
#include "tst_ui_profileeditor.moc"
