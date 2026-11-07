#include <QtTest>

class TstUiAccessibility : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T24."); }
};

QTEST_MAIN(TstUiAccessibility)
#include "tst_ui_accessibility.moc"
