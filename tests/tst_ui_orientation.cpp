#include <QtTest>

class TstUiOrientation : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T11."); }
};

QTEST_MAIN(TstUiOrientation)
#include "tst_ui_orientation.moc"
