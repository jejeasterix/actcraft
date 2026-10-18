// Talon L7-P0 : rempli par L7-P1.
#include <QtTest>

class TstFeaturesSpotlight : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L7-P0 : rempli par L7-P1."); }
};

QTEST_GUILESS_MAIN(TstFeaturesSpotlight)
#include "tst_features_spotlight.moc"
