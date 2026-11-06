#include <QtTest>

class TstUiWebpolicy : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T29."); }
};

QTEST_MAIN(TstUiWebpolicy)
#include "tst_ui_webpolicy.moc"
