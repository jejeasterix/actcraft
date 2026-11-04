#include <QtTest>

class TstUiLabels : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T17."); }
};

QTEST_MAIN(TstUiLabels)
#include "tst_ui_labels.moc"
