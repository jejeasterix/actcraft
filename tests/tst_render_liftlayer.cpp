#include <QTest>

class TstRenderLiftlayer : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L0-S : rempli par L3-T10."); }
};

QTEST_MAIN(TstRenderLiftlayer)
#include "tst_render_liftlayer.moc"
