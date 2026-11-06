#include <QtTest>

class TstUiBroadcast : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T32."); }
};

QTEST_MAIN(TstUiBroadcast)
#include "tst_ui_broadcast.moc"
