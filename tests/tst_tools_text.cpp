#include <QtTest>

class TstToolsText : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L4-T1 : rempli par L4-T22."); }
};

QTEST_GUILESS_MAIN(TstToolsText)
#include "tst_tools_text.moc"
