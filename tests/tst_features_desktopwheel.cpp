// Talon L7-P0 : rempli par L7-P9.
#include <QtTest>

class TstFeaturesDesktopWheel : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L7-P0 : rempli par L7-P9."); }
};

QTEST_GUILESS_MAIN(TstFeaturesDesktopWheel)
#include "tst_features_desktopwheel.moc"
