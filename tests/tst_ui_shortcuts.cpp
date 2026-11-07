#include <QtTest>

class TstUiShortcuts : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T23."); }
};

QTEST_MAIN(TstUiShortcuts)
#include "tst_ui_shortcuts.moc"
