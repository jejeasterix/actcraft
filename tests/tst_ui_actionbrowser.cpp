#include <QtTest>

class TstUiActionbrowser : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T11."); }
};

QTEST_MAIN(TstUiActionbrowser)
#include "tst_ui_actionbrowser.moc"
