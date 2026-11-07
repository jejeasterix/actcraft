#include <QtTest>

class TstUiHelppages : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T17 et L11-T18."); }
};

QTEST_MAIN(TstUiHelppages)
#include "tst_ui_helppages.moc"
