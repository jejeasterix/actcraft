#include <QtTest>

class TstUiRecents : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T24."); }
};

QTEST_MAIN(TstUiRecents)
#include "tst_ui_recents.moc"
