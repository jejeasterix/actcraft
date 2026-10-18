#include <QTest>

class TstUiInputpath : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L0-S : rempli par L0-S7."); }
};

QTEST_MAIN(TstUiInputpath)
#include "tst_ui_inputpath.moc"
