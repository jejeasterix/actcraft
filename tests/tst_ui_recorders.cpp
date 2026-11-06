#include <QtTest>

class TstUiRecorders : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T28."); }
};

QTEST_MAIN(TstUiRecorders)
#include "tst_ui_recorders.moc"
