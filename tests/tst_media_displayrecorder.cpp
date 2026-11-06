#include <QtTest>

class TstMediaDisplayrecorder : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T27."); }
};

QTEST_MAIN(TstMediaDisplayrecorder)
#include "tst_media_displayrecorder.moc"
