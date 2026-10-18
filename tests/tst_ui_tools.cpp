#include <QTest>

class TstUiTools : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L0-S : rempli par L3-T14."); }
};

QTEST_MAIN(TstUiTools)
#include "tst_ui_tools.moc"
