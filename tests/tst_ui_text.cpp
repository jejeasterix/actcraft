#include <QtTest>

class TstUiText : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L4-T1 : rempli par L4-T21."); }
};

QTEST_MAIN(TstUiText)
#include "tst_ui_text.moc"
