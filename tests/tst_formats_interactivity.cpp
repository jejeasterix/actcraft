#include <QtTest>

class TstFormatsInteractivity : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T4."); }
};

QTEST_MAIN(TstFormatsInteractivity)
#include "tst_formats_interactivity.moc"
