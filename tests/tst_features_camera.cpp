// Talon L7-P0 : rempli par L7-P9.
#include <QtTest>

class TstFeaturesCamera : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L7-P0 : rempli par L7-P9."); }
};

QTEST_MAIN(TstFeaturesCamera)
#include "tst_features_camera.moc"
