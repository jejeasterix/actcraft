#include <QtTest>

class TstUiDragdropactions : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T13."); }
};

QTEST_MAIN(TstUiDragdropactions)
#include "tst_ui_dragdropactions.moc"
