#include <QtTest>

class TstUiSecondpalette : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T9."); }
};

QTEST_MAIN(TstUiSecondpalette)
#include "tst_ui_secondpalette.moc"
