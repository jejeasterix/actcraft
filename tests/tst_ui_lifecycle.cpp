#include <QtTest>

class TstUiLifecycle : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T13."); }
};

QTEST_MAIN(TstUiLifecycle)
#include "tst_ui_lifecycle.moc"
