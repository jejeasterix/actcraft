#include <QTest>

class TstToolsEraser : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L0-S : rempli par L3-T7."); }
};

QTEST_GUILESS_MAIN(TstToolsEraser)
#include "tst_tools_eraser.moc"
