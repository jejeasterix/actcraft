#include <QtTest>

class TstUiUpdatechecker : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T21."); }
};

QTEST_MAIN(TstUiUpdatechecker)
#include "tst_ui_updatechecker.moc"
