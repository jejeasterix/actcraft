#include <QtTest>

class TstFormatsLot4 : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L4-T1 : rempli par L4-T6."); }
};

QTEST_GUILESS_MAIN(TstFormatsLot4)
#include "tst_formats_lot4.moc"
