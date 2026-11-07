#include <QtTest>

class TstUiNarrowlayout : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T10."); }
};

QTEST_MAIN(TstUiNarrowlayout)
#include "tst_ui_narrowlayout.moc"
