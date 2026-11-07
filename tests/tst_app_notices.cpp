#include <QtTest>

class TstAppNotices : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T25."); }
};

QTEST_GUILESS_MAIN(TstAppNotices)
#include "tst_app_notices.moc"
