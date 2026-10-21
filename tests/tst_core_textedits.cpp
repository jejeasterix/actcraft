#include <QtTest>

class TstCoreTextedits : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L4-T1 : rempli par L4-T16."); }
};

QTEST_GUILESS_MAIN(TstCoreTextedits)
#include "tst_core_textedits.moc"
