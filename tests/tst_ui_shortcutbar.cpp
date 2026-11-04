#include <QtTest>

class TstUiShortcutbar : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T9."); }
};

QTEST_MAIN(TstUiShortcutbar)
#include "tst_ui_shortcutbar.moc"
