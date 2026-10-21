#include <QtTest>

class TstToolsShape : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L4-T1 : rempli par L4-T8."); }
};

QTEST_GUILESS_MAIN(TstToolsShape)
#include "tst_tools_shape.moc"
