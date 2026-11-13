#include <actcraft/ui/appinfo.h>

#include <QFile>
#include <QtTest>

// Version affichée, règle du versionCode et journal des versions (L11-T28). Le numéro lui-même suit
// project(VERSION).
class TstAppVersionInfo : public QObject
{
    Q_OBJECT
private slots:
    void versionMatchesProject()
    {
        QCOMPARE(actcraft::ui::AppInfo().version(), QStringLiteral(ACTCRAFT_PROJECT_VERSION));
    }

    void versionCodeFollowsTheVersion()
    {
        const QStringList parts = actcraft::ui::AppInfo().version().split(QLatin1Char('.'));
        QCOMPARE(parts.size(), 3);
        const int expected = parts[0].toInt() * 10000 + parts[1].toInt() * 100 + parts[2].toInt();
        QCOMPARE(ACTCRAFT_VERSION_CODE, expected);
    }

    void ruleGivesTheExpectedCodes()
    {
        QCOMPARE(ACTCRAFT_CODE_1_0_0, 10000);
        QCOMPARE(ACTCRAFT_CODE_1_2_3, 10203);
        QCOMPARE(ACTCRAFT_CODE_0_10_0, 1000);
        QVERIFY(ACTCRAFT_CODE_1_0_0 > ACTCRAFT_CODE_0_10_0); // la mise à jour par-dessus passe
    }

    void changelogStartsWithTheNextVersion()
    {
        QFile f(QStringLiteral(ACTCRAFT_SOURCE_DIR "/CHANGELOG.md"));
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString text = QString::fromUtf8(f.readAll());
        const qsizetype first = text.indexOf(QStringLiteral("\n## "));
        QVERIFY(first >= 0);
        QVERIFY(text.mid(first + 1).startsWith(QStringLiteral("## 1.0.0 — ")));
    }
};

QTEST_GUILESS_MAIN(TstAppVersionInfo)
#include "tst_app_versioninfo.moc"
