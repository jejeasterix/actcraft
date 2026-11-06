#include <QtTest>

class TstMediaPlayer : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T24."); }
};

QTEST_MAIN(TstMediaPlayer)
#include "tst_media_player.moc"
