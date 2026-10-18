// Talon L7-P0 : rempli par L7-P3.
#include <QtTest>

class TstFeaturesMathTools : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L7-P0 : rempli par L7-P3."); }
};

QTEST_GUILESS_MAIN(TstFeaturesMathTools)
#include "tst_features_mathtools.moc"
