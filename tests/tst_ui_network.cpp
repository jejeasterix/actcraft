#include <QtTest>

class TstUiNetwork : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T28."); }
};

QTEST_MAIN(TstUiNetwork)
#include "tst_ui_network.moc"
