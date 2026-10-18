// Talon L7-P0 : rempli par L7-P2.
#include <QtTest>

class TstFeaturesTransitions : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L7-P0 : rempli par L7-P2."); }
};

QTEST_GUILESS_MAIN(TstFeaturesTransitions)
#include "tst_features_transitions.moc"
