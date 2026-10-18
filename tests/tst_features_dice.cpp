// Talon L7-P0 : rempli par L7-P5.
#include <QtTest>

class TstFeaturesDice : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L7-P0 : rempli par L7-P5."); }
};

QTEST_GUILESS_MAIN(TstFeaturesDice)
#include "tst_features_dice.moc"
