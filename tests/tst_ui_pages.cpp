#include <QtTest>

class TstUiPages : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L4-T1 : rempli par L4-T18."); }
};

QTEST_MAIN(TstUiPages)
#include "tst_ui_pages.moc"
