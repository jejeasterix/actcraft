#include <QtTest>

class TstUiPagethumbnails : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L4-T1 : rempli par L4-T17."); }
};

QTEST_MAIN(TstUiPagethumbnails)
#include "tst_ui_pagethumbnails.moc"
