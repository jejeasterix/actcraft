#include <QtTest>

class TstAppVersioninfo : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T28."); }
};

QTEST_GUILESS_MAIN(TstAppVersioninfo)
#include "tst_app_versioninfo.moc"
