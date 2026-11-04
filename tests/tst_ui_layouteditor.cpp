#include <QtTest>

class TstUiLayouteditor : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T26."); }
};

QTEST_MAIN(TstUiLayouteditor)
#include "tst_ui_layouteditor.moc"
