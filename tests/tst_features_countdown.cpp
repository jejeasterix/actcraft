// Talon L7-P0 : rempli par L7-P5.
#include <QtTest>

class TstFeaturesCountdown : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L7-P0 : rempli par L7-P5."); }
};

QTEST_GUILESS_MAIN(TstFeaturesCountdown)
#include "tst_features_countdown.moc"
