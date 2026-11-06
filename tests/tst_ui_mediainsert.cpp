#include <QtTest>

class TstUiMediainsert : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T23."); }
};

QTEST_MAIN(TstUiMediainsert)
#include "tst_ui_mediainsert.moc"
