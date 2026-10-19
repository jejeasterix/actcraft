#include <QtTest>

class TstRenderEquation : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L7-S8 : rempli par L7-T25."); }
};

QTEST_MAIN(TstRenderEquation)
#include "tst_render_equation.moc"
