#include <actcraft/actions/actions.h>
#include <actcraft/bench/bench.h>
#include <actcraft/core/core.h>
#include <actcraft/features/features.h>
#include <actcraft/formats/formats.h>
#include <actcraft/input/input.h>
#include <actcraft/render/render.h>
#include <actcraft/tools/tools.h>

#include <QTest>

// Chaque module se lie et s'appelle seul : garde-fou de la structure du § 5 du BRIEF.
class TstModules : public QObject
{
    Q_OBJECT

private slots:
    void eachModuleLinks()
    {
        QCOMPARE(QString::fromLatin1(actcraft::core::moduleId()), QStringLiteral("core"));
        QCOMPARE(QString::fromLatin1(actcraft::formats::moduleId()), QStringLiteral("formats"));
        QCOMPARE(QString::fromLatin1(actcraft::actions::moduleId()), QStringLiteral("actions"));
        QCOMPARE(QString::fromLatin1(actcraft::input::moduleId()), QStringLiteral("input"));
        QCOMPARE(QString::fromLatin1(actcraft::render::moduleId()), QStringLiteral("render"));
        QCOMPARE(QString::fromLatin1(actcraft::tools::moduleId()), QStringLiteral("tools"));
        QCOMPARE(QString::fromLatin1(actcraft::features::moduleId()), QStringLiteral("features"));
        QCOMPARE(QString::fromLatin1(actcraft::bench::moduleId()), QStringLiteral("bench"));
    }
};

QTEST_GUILESS_MAIN(TstModules)
#include "tst_modules.moc"
