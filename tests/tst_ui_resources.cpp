#include <QtTest>

class TstUiResources : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T20."); }
};

QTEST_MAIN(TstUiResources)
#include "tst_ui_resources.moc"
