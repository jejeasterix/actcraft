// Talon L7-P0 : rempli par L7-P8.
#include <QtTest>

class TstFeaturesHandwriting : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L7-P0 : rempli par L7-P8."); }
};

QTEST_GUILESS_MAIN(TstFeaturesHandwriting)
#include "tst_features_handwriting.moc"
