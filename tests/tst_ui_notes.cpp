#include <QtTest>

class TstUiNotes : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T16."); }
};

QTEST_MAIN(TstUiNotes)
#include "tst_ui_notes.moc"
