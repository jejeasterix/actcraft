#include <actcraft/actions/commandregistry.h>
#include <actcraft/actions/layout.h>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace actcraft::actions;

namespace {

const QByteArray kLayout = R"({
  "format": "actcraft-layout/1",
  "menus": [ { "id": "file", "label": "Fichier", "items": ["file.quit", "-", "help.about"] } ],
  "palette": [ { "group": "pages", "items": ["page.next"] }, { "group": "couleurs" } ],
  "shortcuts": { "page.next": "Space" },
  "colors": ["#E53935", "#000000"],
  "penWidths": [2, 4]
})";

bool writeFile(const QString& path, const QByteArray& data)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}

} // namespace

// Disposition décrite par des données (règle 3 du BRIEF, DA1, N2-5).
class TstActionsLayout : public QObject
{
    Q_OBJECT

private slots:
    void parsesEverySection()
    {
        Layout layout;
        QString error;
        QVERIFY2(parseLayout(kLayout, layout, error), qPrintable(error));
        QCOMPARE(layout.menus.size(), std::size_t{1});
        QCOMPARE(layout.menus[0].id, QStringLiteral("file"));
        QCOMPARE(layout.menus[0].label, QStringLiteral("Fichier"));
        QCOMPARE(layout.menus[0].items, (QStringList{QStringLiteral("file.quit"), QStringLiteral("-"),
                                                     QStringLiteral("help.about")}));
        QCOMPARE(layout.palette.size(), std::size_t{2});
        QCOMPARE(layout.palette[0].group, QStringLiteral("pages"));
        QVERIFY(layout.palette[1].items.isEmpty());
        QCOMPARE(layout.shortcuts.value(QStringLiteral("page.next")), QStringLiteral("Space"));
        QCOMPARE(layout.colors, (QStringList{QStringLiteral("#E53935"), QStringLiteral("#000000")}));
        QCOMPARE(layout.penWidths, (QList<double>{2, 4}));
    }

    void rejectsInvalidLayouts_data()
    {
        QTest::addColumn<QByteArray>("json");
        QTest::addColumn<QString>("expected");
        QTest::newRow("format") << QByteArray(R"({"format":"actcraft-layout/9"})")
                                << QStringLiteral("format");
        QTest::newRow("menu") << QByteArray(R"({"format":"actcraft-layout/1","menus":[{"id":"file"}]})")
                              << QStringLiteral("menu 1");
        QTest::newRow("couleur") << QByteArray(R"({"format":"actcraft-layout/1","colors":["rouge"]})")
                                 << QStringLiteral("couleur invalide");
        QTest::newRow("epaisseur") << QByteArray(R"({"format":"actcraft-layout/1","penWidths":[0]})")
                                   << QStringLiteral("épaisseur invalide");
        QTest::newRow("groupe") << QByteArray(R"({"format":"actcraft-layout/1","palette":[{"items":[]}]})")
                                << QStringLiteral("groupe 1");
    }

    void rejectsInvalidLayouts()
    {
        QFETCH(QByteArray, json);
        QFETCH(QString, expected);
        Layout layout;
        layout.colors << QStringLiteral("#123456");
        QString error;
        QVERIFY(!parseLayout(json, layout, error));
        QVERIFY2(error.contains(expected), qPrintable(error));
        QCOMPARE(layout.colors, QStringList{QStringLiteral("#123456")}); // inchangée en cas d'erreur
    }

    void profileFileWinsOverResource()
    {
        QTemporaryDir profile;
        QVERIFY(profile.isValid());
        QVERIFY(writeFile(profile.filePath(QStringLiteral("layout.json")), kLayout));
        QString used;
        const QByteArray data =
            readLayeredFile(QStringLiteral("layout.json"), profile.path(), dataResourceDir(), &used);
        QCOMPARE(used, profile.path() + QStringLiteral("/layout.json"));
        QCOMPARE(data, kLayout);
    }

    void fallsBackToResourceThenToNothing()
    {
        const QString absent = QDir::temp().filePath(QStringLiteral("actcraft-profil-absent-4f2a"));
        QString used;
        QVERIFY(!readLayeredFile(QStringLiteral("layout.json"), absent, dataResourceDir(), &used).isEmpty());
        QCOMPARE(used, dataResourceDir() + QStringLiteral("/layout.json"));
        QVERIFY(readLayeredFile(QStringLiteral("absent.json"), absent, dataResourceDir(), &used).isEmpty());
        QVERIFY(used.isEmpty());
    }

    void defaultLayoutMatchesLotOne()
    {
        Layout layout;
        QString error;
        const QByteArray json = readLayeredFile(QStringLiteral("layout.json"), QString(), dataResourceDir());
        QVERIFY2(parseLayout(json, layout, error), qPrintable(error));
        QStringList labels;
        for (const MenuSpec& menu : layout.menus) {
            labels << menu.label;
        }
        QCOMPARE(labels, (QStringList{QStringLiteral("Fichier"), QStringLiteral("Éditer"),
                                      QStringLiteral("Affichage"), QStringLiteral("Insérer"),
                                      QStringLiteral("Outils"), QStringLiteral("Aide")}));
        QStringList groups;
        for (const PaletteGroup& group : layout.palette) {
            groups << group.group;
        }
        QCOMPARE(groups, (QStringList{QStringLiteral("commandes"), QStringLiteral("pages"),
                                      QStringLiteral("couleurs"), QStringLiteral("epaisseur"),
                                      QStringLiteral("outils")}));
        QCOMPARE(layout.colors.size(), qsizetype{16});
        QCOMPARE(layout.colors.first(), QStringLiteral("#E53935"));
        QCOMPARE(layout.penWidths, (QList<double>{2, 4, 6, 8}));
        CommandRegistry registry;
        QVERIFY2(registry.loadDeclarations(defaultCommandsJson(), error), qPrintable(error));
        QCOMPARE(unknownCommands(layout, registry), QStringList{});
    }

    void unknownCommandsAreListedOnce()
    {
        Layout layout;
        layout.menus.push_back(MenuSpec{QStringLiteral("m"),
                                        QStringLiteral("M"),
                                        {QStringLiteral("a.b"), QStringLiteral("-"), QStringLiteral("a.b")}});
        layout.palette.push_back(PaletteGroup{QStringLiteral("g"), {QStringLiteral("c.d")}});
        CommandRegistry registry;
        QCOMPARE(unknownCommands(layout, registry),
                 (QStringList{QStringLiteral("a.b"), QStringLiteral("c.d")}));
    }
};

QTEST_GUILESS_MAIN(TstActionsLayout)
#include "tst_actions_layout.moc"
