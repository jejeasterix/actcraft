#include <QtTest>

class TstUiObjecttree : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T14."); }
};

QTEST_MAIN(TstUiObjecttree)
#include "tst_ui_objecttree.moc"
