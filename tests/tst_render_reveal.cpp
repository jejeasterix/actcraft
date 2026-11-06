#include <QtTest>

class TstRenderReveal : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T18."); }
};

QTEST_MAIN(TstRenderReveal)
#include "tst_render_reveal.moc"
