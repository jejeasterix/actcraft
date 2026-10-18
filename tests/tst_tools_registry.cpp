#include <QTest>

class TstToolsRegistry : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L0-S : rempli par L3-T6."); }
};

QTEST_GUILESS_MAIN(TstToolsRegistry)
#include "tst_tools_registry.moc"
