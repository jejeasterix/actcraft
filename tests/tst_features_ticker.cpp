// Talon L7-P0 : rempli par L7-P6.
#include <QtTest>

class TstFeaturesTicker : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L7-P0 : rempli par L7-P6."); }
};

QTEST_GUILESS_MAIN(TstFeaturesTicker)
#include "tst_features_ticker.moc"
