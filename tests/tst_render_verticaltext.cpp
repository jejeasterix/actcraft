#include <QtTest>

class TstRenderVerticaltext : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L4-T1 : rempli par L4-T24."); }
};

QTEST_MAIN(TstRenderVerticaltext)
#include "tst_render_verticaltext.moc"
