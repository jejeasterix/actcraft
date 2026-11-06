#include <QtTest>

class TstUiInteractivityprops : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T17."); }
};

QTEST_MAIN(TstUiInteractivityprops)
#include "tst_ui_interactivityprops.moc"
