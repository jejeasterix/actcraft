#include <QtTest>

class TstToolsInteractivity : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T16."); }
};

QTEST_MAIN(TstToolsInteractivity)
#include "tst_tools_interactivity.moc"
