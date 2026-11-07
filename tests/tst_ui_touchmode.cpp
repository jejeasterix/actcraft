#include <QtTest>

class TstUiTouchmode : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T6."); }
};

QTEST_MAIN(TstUiTouchmode)
#include "tst_ui_touchmode.moc"
