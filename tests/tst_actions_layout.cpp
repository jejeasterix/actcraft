#include <actcraft/actions/commandregistry.h>
#include <actcraft/actions/layout.h>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
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

// Les 54 commandes du lot 5 (53 de L5-T1, plus connector.caps de L5-C18), dans l'ordre de commands.json ; la
// liste de L5-T2 est la même.
const QStringList kLot5{"edit.cut",
                        "edit.copy",
                        "edit.paste",
                        "edit.paste-onto",
                        "edit.duplicate",
                        "object.menu",
                        "object.group",
                        "object.lock",
                        "object.hide",
                        "object.drag-copy",
                        "object.shape-points",
                        "arrange.front",
                        "arrange.back",
                        "arrange.forward",
                        "arrange.backward",
                        "arrange.layer-top",
                        "arrange.layer-middle",
                        "arrange.layer-bottom",
                        "align.left",
                        "align.center-x",
                        "align.right",
                        "align.top",
                        "align.center-y",
                        "align.bottom",
                        "align.width",
                        "align.height",
                        "align.angle",
                        "align.all",
                        "transform.original-size",
                        "transform.fit-width",
                        "transform.fit-height",
                        "transform.fit-page",
                        "transform.mirror-x",
                        "transform.mirror-y",
                        "transform.flip-x",
                        "transform.flip-y",
                        "transform.origin-flip-x",
                        "transform.origin-flip-y",
                        "tool.fill",
                        "tool.connector",
                        "connector.style",
                        "connector.caps", // L5-C18 : extrémités du connecteur
                        "view.pen-modifiers",
                        "pen.shape",
                        "insert.image",
                        "insert.media",
                        "tab.next",
                        "tab.previous",
                        "view.tabs",
                        "export.page-image",
                        "export.pdf",
                        "file.import-pdf",
                        "file.import-powerpoint-images",
                        "file.print"};

// Registre lu des ressources livrées.
CommandRegistry registryFromResources()
{
    CommandRegistry registry;
    QString error;
    if (!registry.loadDeclarations(defaultCommandsJson(), error)) {
        qWarning("%s", qPrintable(error));
    }
    return registry;
}

// Disposition lue des ressources livrées.
Layout layoutFromResources()
{
    Layout layout;
    QString error;
    const QByteArray json = readLayeredFile(QStringLiteral("layout.json"), QString(), dataResourceDir());
    if (!parseLayout(json, layout, error)) {
        qWarning("%s", qPrintable(error));
    }
    return layout;
}

// kLayout (format gardé) avec la liste `objectMenu` donnée ; les sous-menus d'objectMenu sont déclarés en
// ligne, comme ceux de contextMenu (patron du lot 4).
QByteArray layoutWith(const QJsonArray& objectMenu)
{
    QJsonObject root = QJsonDocument::fromJson(kLayout).object();
    root.insert(QStringLiteral("objectMenu"), objectMenu);
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

QJsonObject inlineSubmenu(const QString& id)
{
    return QJsonObject{{QStringLiteral("submenu"), id},
                       {QStringLiteral("label"), id},
                       {QStringLiteral("items"), QJsonArray{QStringLiteral("help.about")}}};
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
        // Lot 5 (L5-T1) : six boutons ajoutés en fin ; les cinq du lot 3 restent en tête.
        QCOMPARE(layout.objectBar.mid(0, 5),
                 (QStringList{"object.move-free", "object.rotate", "object.translucency", "object.enlarge",
                              "object.reduce"}));
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
                                               ">camera", ">math", ">other-tools",
                                               // Lot 6 (L6-T1) : fin du menu, après « Autres outils ».
                                               "-", "spell.check", "launcher.edit", "classroom.lock",
                                               "classroom.password", "-", "profile.edit"}));
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

    void lot5EntriesDeclared()
    {
        QCOMPARE(kLot5.size(), 54);
        const CommandRegistry registry = registryFromResources();
        QSet<QString> shortcuts;
        for (const QString& id : registry.ids()) {
            const CommandInfo* command = registry.find(id);
            QVERIFY(command != nullptr);
            if (command->shortcut.isEmpty()) {
                continue;
            }
            QVERIFY2(!shortcuts.contains(command->shortcut), qPrintable(command->shortcut));
            shortcuts.insert(command->shortcut);
        }
        QSet<QString> seen;
        for (const QString& id : kLot5) {
            QVERIFY2(registry.find(id) != nullptr, qPrintable(id));
            QVERIFY2(!seen.contains(id), qPrintable(id));
            seen.insert(id);
        }
        QCOMPARE(registry.ids().count(QStringLiteral("edit.cut")), 1); // déclarée une seule fois
    }

    // L6-P2 (P13) : les deux entrées du lot 6 forment un groupe final, après la reconnaissance.
    void lot6EntriesInObjectMenu()
    {
        const Layout layout = layoutFromResources();
        const qsizetype properties = layout.objectMenu.indexOf(QStringLiteral("browser.properties"));
        const qsizetype add = layout.objectMenu.indexOf(QStringLiteral("resources.add"));
        QVERIFY(properties > 0 && add == properties + 1);
        QCOMPARE(layout.objectMenu.at(properties - 1), QStringLiteral("-"));
        QCOMPARE(layout.objectMenu.count(QStringLiteral("browser.properties")), 1);
    }

    void objectMenuOrder()
    {
        const Layout layout = layoutFromResources();
        const QStringList expected{"object.shape-points",
                                   "-",
                                   "edit.cut",
                                   "edit.copy",
                                   "edit.paste-onto",
                                   "edit.duplicate",
                                   "edit.delete",
                                   "-",
                                   ">transform",
                                   ">align",
                                   ">arrange",
                                   "-",
                                   "object.lock",
                                   "object.group",
                                   "object.hide",
                                   "object.drag-copy",
                                   "shortcutbar.add-selection",
                                   "-",
                                   "recognition.convert-to-text",
                                   "recognition.convert-to-shape",
                                   "-",
                                   "browser.properties",
                                   "resources.add"};
        QCOMPARE(layout.objectMenu, expected);
        QCOMPARE(layout.objectSubmenus.size(), std::size_t{3});
        QCOMPARE(layout.objectSubmenus[0].items.size(), 13);
    }

    // Patron du lot 4 : les sous-menus d'objectMenu sont déclarés en ligne ; un jeton « >id » sans
    // déclaration et un sous-menu déclaré deux fois sont refusés, avec leur nom.
    void objectMenuSubmenusResolve()
    {
        Layout layout;
        QString error;
        QVERIFY2(parseLayout(layoutWith({inlineSubmenu(QStringLiteral("present")), QStringLiteral("-"),
                                         QStringLiteral("help.about")}),
                             layout, error),
                 qPrintable(error));
        QCOMPARE(layout.objectMenu, (QStringList{QStringLiteral(">present"), QStringLiteral("-"),
                                                 QStringLiteral("help.about")}));
        QCOMPARE(layout.objectSubmenus.size(), std::size_t{1});
        Layout refused;
        QVERIFY(!parseLayout(layoutWith({QStringLiteral(">absent")}), refused, error));
        QVERIFY2(error.contains(QStringLiteral("objectMenu : sous-menu absent inconnu")), qPrintable(error));
        QVERIFY(!parseLayout(
            layoutWith({inlineSubmenu(QStringLiteral("present")), inlineSubmenu(QStringLiteral("present"))}),
            refused, error));
        QVERIFY2(error.contains(QStringLiteral("sous-menu present déclaré deux fois")), qPrintable(error));
        Layout withUnknown;
        QVERIFY2(
            parseLayout(layoutWith({QStringLiteral("nope.cmd"), inlineSubmenu(QStringLiteral("present"))}),
                        withUnknown, error),
            qPrintable(error));
        QCOMPARE(unknownCommands(withUnknown, registryFromResources()),
                 QStringList{QStringLiteral("nope.cmd")});
    }

    void lot6EntriesDeclared()
    {
        CommandRegistry registry;
        QString error;
        QVERIFY2(registry.loadDeclarations(defaultCommandsJson(), error), qPrintable(error));
        const QStringList ids = {
            QStringLiteral("palette.dock-left"),
            QStringLiteral("palette.dock-right"),
            QStringLiteral("palette.dock-top"),
            QStringLiteral("palette.dock-bottom"),
            QStringLiteral("palette.float"),
            QStringLiteral("palette.collapse"),
            QStringLiteral("palette.collapse-full"),
            QStringLiteral("palette.pin"),
            QStringLiteral("palette.icon-size"),
            QStringLiteral("color.more"),
            QStringLiteral("color.pick"),
            QStringLiteral("color.dialog"),
            QStringLiteral("color.add"),
            QStringLiteral("browser.objects"),
            QStringLiteral("browser.properties"),
            QStringLiteral("browser.notes"),
            QStringLiteral("browser.resources"),
            QStringLiteral("profile.choose"),
            QStringLiteral("profile.activate"),
            QStringLiteral("profile.edit"),
            QStringLiteral("profile.save-as"),
            QStringLiteral("profile.import"),
            QStringLiteral("profile.export"),
            QStringLiteral("profile.delete"),
            QStringLiteral("profile.reset"),
            QStringLiteral("mode.design"),
            QStringLiteral("view.dashboard"),
            QStringLiteral("file.recent-clear"),
            QStringLiteral("resources.mine"),
            QStringLiteral("resources.shared"),
            QStringLiteral("resources.other"),
            QStringLiteral("resources.insert"),
            QStringLiteral("resources.add"),
            QStringLiteral("resources.stamp"),
            QStringLiteral("resources.new-folder"),
            QStringLiteral("resources.rename"),
            QStringLiteral("resources.delete"),
            QStringLiteral("classroom.lock"),
            QStringLiteral("classroom.password"),
            QStringLiteral("spell.check"),
            QStringLiteral("launcher.run"),
            QStringLiteral("launcher.edit"),
            QStringLiteral("help.contents"),
            QStringLiteral("shortcutbar.insert"),
            QStringLiteral("shortcutbar.add-selection"),
            QStringLiteral("settings.language"),
        };
        QCOMPARE(ids.size(), 46);
        for (const QString& id : ids) {
            QVERIFY2(registry.find(id) != nullptr, qPrintable(id));
        }
        QCOMPARE(registry.find(QStringLiteral("help.contents"))->shortcut, QStringLiteral("F1"));
        QVERIFY(registry.find(QStringLiteral("help.website"))->shortcut.isEmpty());
        QCOMPARE(registry.find(QStringLiteral("launcher.edit"))->platforms, Platforms(Platform::Windows));
        QVERIFY(registry.find(QStringLiteral("mode.design"))->checkable);
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
