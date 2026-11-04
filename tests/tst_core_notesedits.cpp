#include <QtTest>

class TstCoreNotesedits : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T16."); }
};

QTEST_MAIN(TstCoreNotesedits)
#include "tst_core_notesedits.moc"
