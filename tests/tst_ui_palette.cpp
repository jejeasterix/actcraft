#include <QtTest>

class TstUiPalette : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T7."); }
};

QTEST_MAIN(TstUiPalette)
#include "tst_ui_palette.moc"
