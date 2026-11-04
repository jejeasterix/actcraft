#include <QtTest>

class TstUiSpelling : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T34."); }
};

QTEST_MAIN(TstUiSpelling)
#include "tst_ui_spelling.moc"
