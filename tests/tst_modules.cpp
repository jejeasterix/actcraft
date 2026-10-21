#include <actcraft/actions/actions.h>
#include <actcraft/bench/bench.h>
#include <actcraft/core/core.h>
#include <actcraft/features/features.h>
#include <actcraft/formats/formats.h>
#include <actcraft/input/input.h>
#include <actcraft/render/render.h>
#include <actcraft/tools/tools.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
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

    // Règle de modularité 2 (L3-T14) : un outil ne connaît que des interfaces ; ui/ les branche sur render/.
    void toolsSeeNoRenderItem()
    {
        const QString root =
            QFileInfo(QString::fromUtf8(__FILE__)).absolutePath() + QStringLiteral("/../tools");
        int files = 0;
        for (const char* folder : {"/include", "/src"}) {
            const QString path = root + QString::fromLatin1(folder);
            QVERIFY2(QDir(path).exists(), qPrintable(path));
            QDirIterator it(path, {QStringLiteral("*.h"), QStringLiteral("*.cpp")}, QDir::Files,
                            QDirIterator::Subdirectories);
            while (it.hasNext()) {
                const QString file = it.next();
                QFile source(file);
                QVERIFY2(source.open(QIODevice::ReadOnly), qPrintable(file));
                const QByteArray text = source.readAll();
                QVERIFY2(!text.contains("#include <actcraft/render/"), qPrintable(file));
                QVERIFY2(!text.contains("#include <actcraft/ui/"), qPrintable(file));
                ++files;
            }
        }
        QVERIFY(files >= 10);
    }
};

QTEST_GUILESS_MAIN(TstModules)
#include "tst_modules.moc"
