#include <QtTest>

class TstUiLinkdialogs : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T22."); }
};

QTEST_MAIN(TstUiLinkdialogs)
#include "tst_ui_linkdialogs.moc"
