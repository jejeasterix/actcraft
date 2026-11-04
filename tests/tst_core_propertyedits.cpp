#include <QtTest>

class TstCorePropertyedits : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T11."); }
};

QTEST_MAIN(TstCorePropertyedits)
#include "tst_core_propertyedits.moc"
