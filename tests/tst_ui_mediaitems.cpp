#include <QtTest>

class TstUiMediaitems : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T25."); }
};

QTEST_MAIN(TstUiMediaitems)
#include "tst_ui_mediaitems.moc"
