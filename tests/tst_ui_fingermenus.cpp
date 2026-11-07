#include <QtTest>

class TstUiFingermenus : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T12."); }
};

QTEST_MAIN(TstUiFingermenus)
#include "tst_ui_fingermenus.moc"
