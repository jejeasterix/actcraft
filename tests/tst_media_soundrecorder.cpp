#include <QtTest>

class TstMediaSoundrecorder : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T26."); }
};

QTEST_MAIN(TstMediaSoundrecorder)
#include "tst_media_soundrecorder.moc"
