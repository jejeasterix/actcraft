#include <QTest>

class TstRenderStrokeshape : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L0-S : rempli par L3-T11."); }
};

QTEST_MAIN(TstRenderStrokeshape)
#include "tst_render_strokeshape.moc"
