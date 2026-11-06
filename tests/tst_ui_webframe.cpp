#include <QtTest>

class TstUiWebframe : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L8 : rempli par L8-T30."); }
};

QTEST_MAIN(TstUiWebframe)
#include "tst_ui_webframe.moc"
