#include <QtTest>

class TstRenderGridoverlay : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L4-T1 : rempli par L4-T7."); }
};

QTEST_MAIN(TstRenderGridoverlay)
#include "tst_render_gridoverlay.moc"
