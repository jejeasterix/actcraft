#include <QtTest>

class TstCoreObjectactionsEdit : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T8."); }
};

QTEST_MAIN(TstCoreObjectactionsEdit)
#include "tst_core_objectactions_edit.moc"
