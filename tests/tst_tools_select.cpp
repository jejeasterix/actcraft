#include <QTest>

class TstToolsSelect : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L0-S : rempli par L3-T8."); }
};

QTEST_GUILESS_MAIN(TstToolsSelect)
#include "tst_tools_select.moc"
