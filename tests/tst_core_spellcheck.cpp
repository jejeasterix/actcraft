#include <QtTest>

class TstCoreSpellcheck : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T33."); }
};

QTEST_MAIN(TstCoreSpellcheck)
#include "tst_core_spellcheck.moc"
