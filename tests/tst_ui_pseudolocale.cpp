#include <QtTest>

class TstUiPseudolocale : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T19."); }
};

QTEST_MAIN(TstUiPseudolocale)
#include "tst_ui_pseudolocale.moc"
