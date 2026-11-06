#include <QtTest>

class TstFormatsSamplesInteractivity : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T19."); }
};

QTEST_MAIN(TstFormatsSamplesInteractivity)
#include "tst_formats_samples_interactivity.moc"
