// Talon L7-P0 : rempli par L7-P7.
#include <QtTest>

class TstFeaturesRecognitionSession : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L7-P0 : rempli par L7-P7."); }
};

QTEST_GUILESS_MAIN(TstFeaturesRecognitionSession)
#include "tst_features_recognitionsession.moc"
