#include <QtTest>

class TstFeaturesEquationEditor : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L7-S8 : rempli par L7-T26."); }
};

QTEST_GUILESS_MAIN(TstFeaturesEquationEditor)
#include "tst_features_equationeditor.moc"
