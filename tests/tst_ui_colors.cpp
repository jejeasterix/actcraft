#include <QtTest>

class TstUiColors : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T8."); }
};

QTEST_MAIN(TstUiColors)
#include "tst_ui_colors.moc"
