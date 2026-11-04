#include <QtTest>

class TstUiPropertymodel : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T12."); }
};

QTEST_MAIN(TstUiPropertymodel)
#include "tst_ui_propertymodel.moc"
