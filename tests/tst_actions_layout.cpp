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
        // Socle des lots 3 et 7 (L0-S4) : la palette gagne les groupes « edition » et « presentation » après
        // « outils ». Les cinq premiers groupes, ceux du lot 1, ne bougent pas.
        // Amendement 1 du lot 4 : le groupe « formes-texte » est ajouté en fin de palette (D19).
        QCOMPARE(groups, (QStringList{QStringLiteral("commandes"), QStringLiteral("pages"),
                                      QStringLiteral("couleurs"), QStringLiteral("epaisseur"),
                                      QStringLiteral("outils"), QStringLiteral("edition"),
                                      QStringLiteral("presentation"), QStringLiteral("formes-texte")}));
        QCOMPARE(layout.colors.size(), qsizetype{16});
        QCOMPARE(layout.colors.first(), QStringLiteral("#E53935"));
        QCOMPARE(layout.penWidths, (QList<double>{2, 4, 6, 8}));
        CommandRegistry registry;
        QVERIFY2(registry.loadDeclarations(defaultCommandsJson(), error), qPrintable(error));
        QCOMPARE(unknownCommands(layout, registry), QStringList{});
    }

    void submenuItems()
    {
        const QByteArray json = R"({"format": "actcraft-layout/1", "menus": [{"id": "edit", "label": "Éditer",
            "items": ["edit.undo", {"submenu": "clear", "label": "Effacer", "items": ["clear.page"]}]}],
            "palette": [], "colors": [], "penWidths": []})";
        Layout layout;
        QString error;
        QVERIFY2(parseLayout(json, layout, error), qPrintable(error));
        QCOMPARE(layout.menus.size(), std::size_t{1});
        QCOMPARE(layout.menus[0].items, (QStringList{QStringLiteral("edit.undo"), QStringLiteral(">clear")}));
        QCOMPARE(layout.menus[0].submenus.size(), std::size_t{1});
        QCOMPARE(layout.menus[0].submenus[0].id, QStringLiteral("clear"));
        QCOMPARE(layout.menus[0].submenus[0].label, QStringLiteral("Effacer"));
        QCOMPARE(layout.menus[0].submenus[0].items, QStringList{QStringLiteral("clear.page")});
    }

    void objectBarRead()
    {
        const QByteArray json = R"({"format": "actcraft-layout/1", "menus": [], "palette": [], "colors": [],
            "penWidths": [], "objectBar": ["object.enlarge", "object.reduce"]})";
        Layout layout;
        QString error;
        QVERIFY2(parseLayout(json, layout, error), qPrintable(error));
        QCOMPARE(layout.objectBar,
                 (QStringList{QStringLiteral("object.enlarge"), QStringLiteral("object.reduce")}));
    }

    void contextMenuRead()
    {
        const QByteArray json = R"({"format": "actcraft-layout/1", "menus": [], "palette": [], "colors": [],
            "penWidths": [], "contextMenu": ["tool.select", "-", "tool.pen"]})";
        Layout layout;
        QString error;
        QVERIFY2(parseLayout(json, layout, error), qPrintable(error));
        QCOMPARE(layout.contextMenu, (QStringList{QStringLiteral("tool.select"), QStringLiteral("-"),
                                                  QStringLiteral("tool.pen")}));
    }

    void nestedSubmenuRefused()
    {
        const QByteArray json =
            R"({"format": "actcraft-layout/1", "menus": [{"id": "m", "label": "M", "items": [
            {"submenu": "a", "label": "A", "items": [{"submenu": "b", "label": "B", "items": []}]}]}],
            "palette": [], "colors": [], "penWidths": []})";
        Layout layout;
        QString error;
        QVERIFY(!parseLayout(json, layout, error));
        QVERIFY(error.contains(QStringLiteral("sous-menu")));
    }

    void lot3EntriesDeclared()
    {
        CommandRegistry registry;
        QString error;
        QVERIFY2(registry.loadDeclarations(defaultCommandsJson(), error), qPrintable(error));
        const QStringList lot3 = {
            "tool.select",         "tool.highlighter", "tool.eraser",      "edit.undo",
            "edit.redo",           "edit.delete",      "edit.select-all",  "clear.annotations",
            "clear.objects",       "clear.grid",       "clear.background", "clear.page",
            "page.revert",         "view.trash",       "object.move-free", "object.rotate",
            "object.translucency", "object.enlarge",   "object.reduce"};
        for (const QString& id : lot3) {
            QVERIFY2(registry.find(id) != nullptr, qPrintable(id));
        }
        Layout layout;
        const QByteArray json = readLayeredFile(QStringLiteral("layout.json"), QString(), dataResourceDir());
        QVERIFY2(parseLayout(json, layout, error), qPrintable(error));
        QVERIFY(unknownCommands(layout, registry).isEmpty());
        QCOMPARE(layout.objectBar, (QStringList{"object.move-free", "object.rotate", "object.translucency",
                                                "object.enlarge", "object.reduce"}));
        // Amendement 1 du lot 4 : entrées du menu de page ajoutées en fin ; celles du socle restent en tête.
        QCOMPARE(layout.contextMenu.mid(0, 4),
                 (QStringList{"tool.select", "tool.pen", "-", "recognition.convert-to-text"}));
    }

    void lot7EntriesPlaced()
    {
        CommandRegistry registry;
        QString error;
        QVERIFY2(registry.loadDeclarations(defaultCommandsJson(), error), qPrintable(error));
        Layout layout;
        const QByteArray json = readLayeredFile(QStringLiteral("layout.json"), QString(), dataResourceDir());
        QVERIFY2(parseLayout(json, layout, error), qPrintable(error));
        QVERIFY(unknownCommands(layout, registry).isEmpty());
        const auto menu = [&](const QString& id) -> const MenuSpec* {
            for (const MenuSpec& m : layout.menus) {
                if (m.id == id) {
                    return &m;
                }
            }
            return nullptr;
        };
        const MenuSpec* tools = menu(QStringLiteral("tools"));
        const MenuSpec* view = menu(QStringLiteral("view"));
        const MenuSpec* insert = menu(QStringLiteral("insert"));
        QVERIFY(tools && view && insert);
        QStringList submenuIds;
        for (const MenuSpec& s : tools->submenus) {
            submenuIds << s.id;
        }
        QCOMPARE(submenuIds, (QStringList{"spot", "camera", "math", "other-tools"}));
        QVERIFY(view->items.contains(QStringLiteral("view.page-zoom")));
        QVERIFY(view->items.contains(QStringLiteral("view.page-effects")));
        QVERIFY(insert->items.contains(QStringLiteral("insert.date-time")));
        QVERIFY(insert->items.contains(QStringLiteral("insert.equation")));
        QStringList groups;
        for (const PaletteGroup& g : layout.palette) {
            groups << g.group;
        }
        QVERIFY(groups.contains(QStringLiteral("presentation")));
    }

    void toolsMenuFollowsF750()
    {
        Layout layout;
        QString error;
        const QByteArray json = readLayeredFile(QStringLiteral("layout.json"), QString(), dataResourceDir());
        QVERIFY2(parseLayout(json, layout, error), qPrintable(error));
        for (const MenuSpec& m : layout.menus) {
            if (m.id == QLatin1String("tools")) {
                QCOMPARE(m.items, (QStringList{"desktop.annotate", "desktop.tools", "-", "tool.pen",
                                               "tool.highlighter", "tool.eraser", "-", "tool.handwriting",
                                               "tool.shape-recognition", "-", "present.curtain", ">spot",
                                               ">camera", ">math", ">other-tools"}));
                // Amendement 1 du lot 4, F-750 : « tool.text-vertical » en fin d'« Autres outils ».
                for (const MenuSpec& sub : m.submenus) {
                    if (sub.id == QLatin1String("other-tools")) {
                        QCOMPARE(sub.items, (QStringList{"tool.keyboard", "tool.clock", "tool.ticker",
                                                         "tool.web-browser", "tool.text-vertical"}));
                        return;
                    }
                }
                QFAIL("sous-menu other-tools absent");
            }
        }
        QFAIL("menu tools absent");
    }

    void contextMenuAcceptsSubmenus()
    {
        const QByteArray json = R"({"format": "actcraft-layout/1", "menus": [], "palette": [], "colors": [],
            "penWidths": [], "contextMenu": ["tool.select", "-", {"submenu": "page-insert", "label": "Insérer une page",
            "items": ["page.insert-before", "page.insert-after"]}]})";
        Layout layout;
        QString error;
        QVERIFY2(parseLayout(json, layout, error), qPrintable(error));
        QCOMPARE(layout.contextMenu, (QStringList{QStringLiteral("tool.select"), QStringLiteral("-"),
                                                  QStringLiteral(">page-insert")}));
        QCOMPARE(layout.contextSubmenus.size(), std::size_t{1});
        QCOMPARE(layout.contextSubmenus.front().items,
                 (QStringList{QStringLiteral("page.insert-before"), QStringLiteral("page.insert-after")}));
    }

    void unknownCommandsAreListedOnce()
    {
        Layout layout;
        layout.menus.push_back(MenuSpec{QStringLiteral("m"),
                                        QStringLiteral("M"),
                                        {QStringLiteral("a.b"), QStringLiteral("-"), QStringLiteral("a.b")},
                                        {}});
        layout.palette.push_back(PaletteGroup{QStringLiteral("g"), {QStringLiteral("c.d")}});
        CommandRegistry registry;
        QCOMPARE(unknownCommands(layout, registry),
                 (QStringList{QStringLiteral("a.b"), QStringLiteral("c.d")}));
    }
};

QTEST_GUILESS_MAIN(TstActionsLayout)
#include "tst_actions_layout.moc"
