#include <QtTest>

class TstUiLaunchers : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T29."); }
};

QTEST_MAIN(TstUiLaunchers)
#include "tst_ui_launchers.moc"
