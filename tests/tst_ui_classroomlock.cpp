#include <QtTest>

class TstUiClassroomlock : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L6 : rempli par L6-T31."); }
};

QTEST_MAIN(TstUiClassroomlock)
#include "tst_ui_classroomlock.moc"
