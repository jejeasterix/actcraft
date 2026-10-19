#include <actcraft/tools/userstate.h>

#include <QString>
#include <QTest>

#include <cstddef>

class TstToolsRegistry : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L0-S : rempli par L3-T6."); }

    void openMathToolsKept()
    {
        actcraft::tools::UserState user;
        QCOMPARE(user.id(), actcraft::input::kFirstUser);
        QVERIFY(user.openMathTools().empty());
        user.setOpenMathTools({QStringLiteral("ruler"), QStringLiteral("compass")});
        QCOMPARE(user.openMathTools().size(), std::size_t{2});
        QCOMPARE(user.openMathTools()[1], QStringLiteral("compass"));
    }
};

QTEST_GUILESS_MAIN(TstToolsRegistry)
#include "tst_tools_registry.moc"
