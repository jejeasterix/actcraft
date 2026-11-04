#include <QtTest>

class TstUiLanguage : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T36."); }
};

QTEST_MAIN(TstUiLanguage)
#include "tst_ui_language.moc"
