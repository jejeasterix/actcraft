#include <QtTest>

class TstFormatsFlipchartInteractivity : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T5."); }
};

QTEST_MAIN(TstFormatsFlipchartInteractivity)
#include "tst_formats_flipchart_interactivity.moc"
