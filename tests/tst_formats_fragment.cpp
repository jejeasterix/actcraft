#include <QtTest>

class TstFormatsFragment : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T19."); }
};

QTEST_MAIN(TstFormatsFragment)
#include "tst_formats_fragment.moc"
