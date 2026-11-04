#include <QtTest>

class TstFormatsLot6 : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T10."); }
};

QTEST_MAIN(TstFormatsLot6)
#include "tst_formats_lot6.moc"
