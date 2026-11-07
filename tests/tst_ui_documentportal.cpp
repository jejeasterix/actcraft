#include <QtTest>

class TstUiDocumentportal : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T14."); }
};

QTEST_MAIN(TstUiDocumentportal)
#include "tst_ui_documentportal.moc"
