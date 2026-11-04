#include <QtTest>

class TstUiDesignmode : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T22."); }
};

QTEST_MAIN(TstUiDesignmode)
#include "tst_ui_designmode.moc"
