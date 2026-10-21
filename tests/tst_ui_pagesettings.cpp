#include <QtTest>

class TstUiPagesettings : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L4-T1 : rempli par L4-T13."); }
};

QTEST_MAIN(TstUiPagesettings)
#include "tst_ui_pagesettings.moc"
