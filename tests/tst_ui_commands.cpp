#include <actcraft/ui/commands.h>

#include <QFile>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using actcraft::actions::Platform;
using actcraft::ui::Commands;

namespace {

QStringList menuLabels(const Commands& commands)
{
    QStringList out;
    for (const QVariant& menu : commands.menus()) {
        out << menu.toMap().value(QStringLiteral("label")).toString();
    }
    return out;
}

QStringList itemIds(const QVariant& menu)
{
    QStringList out;
    for (const QVariant& item : menu.toMap().value(QStringLiteral("items")).toList()) {
        const QVariantMap map = item.toMap();
        out << (map.value(QStringLiteral("separator")).toBool() ? QStringLiteral("-")
                                                                : map.value(QStringLiteral("id")).toString());
    }
    return out;
}

bool writeFile(const QString& path, const QByteArray& data)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}

} // namespace

// Pont QML du registre de commandes (DA1).
class TstUiCommands : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_emptyProfile; // profil sans disposition : celle des ressources

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY(m_emptyProfile.isValid());
    }

    void defaultMenusHideEmptyOnes()
    {
        Commands c;
        c.setProfileDir(m_emptyProfile.path());
        c.setPlatform(Platform::Windows);
        QVERIFY2(c.errors().isEmpty(), qPrintable(c.errors().join(QLatin1Char('\n'))));
        // Socle des lots 3 et 7 (L0-S4) : Éditer et Insérer ne sont plus vides.
        QCOMPARE(menuLabels(c), (QStringList{QStringLiteral("Fichier"), QStringLiteral("Éditer"),
                                             QStringLiteral("Affichage"), QStringLiteral("Insérer"),
                                             QStringLiteral("Outils"), QStringLiteral("Aide")}));
        QCOMPARE(itemIds(c.menus().at(2)).mid(0, 6),
                 (QStringList{QStringLiteral("view.fullscreen"), QStringLiteral("-"),
                              QStringLiteral("page.previous"), QStringLiteral("page.next"),
                              QStringLiteral("-"), QStringLiteral("view.fit")}));
        const QVariantMap fullscreen =
            c.menus().at(2).toMap().value(QStringLiteral("items")).toList().at(0).toMap();
        QCOMPARE(fullscreen.value(QStringLiteral("label")).toString(), QStringLiteral("Plein écran"));
        QCOMPARE(fullscreen.value(QStringLiteral("shortcut")).toString(), QStringLiteral("F5"));
        QVERIFY(fullscreen.value(QStringLiteral("checkable")).toBool());
        QCOMPARE(c.platformName(), QStringLiteral("windows"));
    }

    void paletteColorsAndWidths()
    {
        Commands c;
        c.setProfileDir(m_emptyProfile.path());
        QStringList groups;
        for (const QVariant& group : c.palette()) {
            groups << group.toMap().value(QStringLiteral("group")).toString();
        }
        // Groupes « edition » et « presentation » ajoutés par le socle (L0-S4), « formes-texte » par le
        // lot 4.
        QCOMPARE(groups, (QStringList{QStringLiteral("commandes"), QStringLiteral("pages"),
                                      QStringLiteral("couleurs"), QStringLiteral("epaisseur"),
                                      QStringLiteral("outils"), QStringLiteral("edition"),
                                      QStringLiteral("presentation"), QStringLiteral("formes-texte")}));
        const QVariantMap first =
            c.palette().at(0).toMap().value(QStringLiteral("items")).toList().at(0).toMap();
        QCOMPARE(first.value(QStringLiteral("id")).toString(), QStringLiteral("menu.main"));
        QCOMPARE(first.value(QStringLiteral("icon")).toString(),
                 QStringLiteral("qrc:/qt/qml/ActCraft/Ui/icons/menu.svg"));
        QCOMPARE(c.colors().size(), qsizetype{16});
        QCOMPARE(c.penWidths(), (QVariantList{2.0, 4.0, 6.0, 8.0}));
    }

    void submenuInMenus()
    {
        Commands commands;
        commands.setProfileDir(m_emptyProfile.path());
        commands.setPlatform(Platform::Windows);
        const QVariantList menus = commands.menus();
        QVariantMap edit;
        for (const QVariant& m : menus) {
            if (m.toMap().value(QStringLiteral("id")).toString() == QLatin1String("edit")) {
                edit = m.toMap();
            }
        }
        QVERIFY(!edit.isEmpty());
        bool found = false;
        for (const QVariant& it : edit.value(QStringLiteral("items")).toList()) {
            const QVariantMap item = it.toMap();
            // Lot 5 : le menu Éditer gagne trois sous-menus (Transformer, Aligner, Organiser) après « Effacer
            // ».
            if (item.value(QStringLiteral("submenu")).toBool() &&
                item.value(QStringLiteral("id")).toString() == QLatin1String("clear")) {
                found = true;
                QCOMPARE(item.value(QStringLiteral("label")).toString(), QStringLiteral("Effacer"));
                QCOMPARE(item.value(QStringLiteral("items")).toList().size(), 5);
            }
        }
        QVERIFY(found);
        QCOMPARE(commands.objectBar().size(), 13); // 5 d'avant le lot 5, 6 du lot 5, 2 du lot 8
    }

    void submenuFilteredByPlatform()
    {
        const auto cameraOf = [](const Commands& c) {
            for (const QVariant& m : c.menus()) {
                if (m.toMap().value(QStringLiteral("id")).toString() != QLatin1String("tools")) {
                    continue;
                }
                for (const QVariant& it : m.toMap().value(QStringLiteral("items")).toList()) {
                    if (it.toMap().value(QStringLiteral("id")).toString() == QLatin1String("camera") &&
                        it.toMap().value(QStringLiteral("submenu")).toBool()) {
                        return it.toMap();
                    }
                }
            }
            return QVariantMap{};
        };
        Commands c;
        c.setProfileDir(m_emptyProfile.path());
        c.setPlatform(Platform::Windows);
        const QVariantMap camera = cameraOf(c);
        QVERIFY(!camera.isEmpty());
        QCOMPARE(camera.value(QStringLiteral("label")).toString(), QStringLiteral("Caméra"));
        QCOMPARE(camera.value(QStringLiteral("items")).toList().size(), 5);
        c.setPlatform(Platform::Android);
        QVERIFY(cameraOf(c).isEmpty());
    }

    void objectBarAndContextMenu()
    {
        Commands c;
        c.setProfileDir(m_emptyProfile.path());
        c.setPlatform(Platform::Windows);
        const QVariantList bar = c.objectBar();
        // Lot 5 (L5-T1, F-521) : les cinq éléments d'avant le lot 5 inchangés en tête, les six du lot à la
        // fin.
        QCOMPARE(bar.size(), 13); // + action.apply, action.remove (lot 8, L8-T1)
        QStringList barIds;
        for (const QVariant& entry : bar) {
            barIds << entry.toMap().value(QStringLiteral("id")).toString();
        }
        QCOMPARE(barIds,
                 (QStringList{"object.move-free", "object.rotate", "object.translucency", "object.enlarge",
                              "object.reduce", "object.menu", "object.group", "arrange.front", "arrange.back",
                              "edit.duplicate", "object.shape-points", "action.apply", "action.remove"}));
        const QVariantMap first = bar.at(0).toMap();
        QCOMPARE(first.value(QStringLiteral("id")).toString(), QStringLiteral("object.move-free"));
        QVERIFY(first.contains(QStringLiteral("label")));
        QVERIFY(first.contains(QStringLiteral("icon")));
        // Amendement 1 du lot 4 : entrées du menu de page ajoutées en fin ; les entrées du socle restent en
        // tête.
        QCOMPARE(itemIds(QVariantMap{{QStringLiteral("items"), c.contextMenu()}}).mid(0, 4),
                 (QStringList{QStringLiteral("tool.select"), QStringLiteral("tool.pen"), QStringLiteral("-"),
                              QStringLiteral("recognition.convert-to-text")}));
        QVERIFY(c.contextMenu().at(2).toMap().value(QStringLiteral("separator")).toBool());
        // Lot 5 : le menu de page finit par l'export de la page en image.
        QCOMPARE(itemIds(QVariantMap{{QStringLiteral("items"), c.contextMenu()}}).last(),
                 QStringLiteral("export.page-image"));
        c.setPlatform(Platform::Android);
        QCOMPARE(itemIds(QVariantMap{{QStringLiteral("items"), c.contextMenu()}}).mid(0, 2),
                 (QStringList{QStringLiteral("tool.select"), QStringLiteral("tool.pen")}));
    }

    // objectMenu est rendu comme menus() : « >transform » devient une entrée de sous-menu portant ses
    // éléments (12 commandes et 1 séparateur, comme dans le menu Éditer).
    void objectMenuRenderedLikeMenus()
    {
        Commands c;
        c.setProfileDir(m_emptyProfile.path());
        c.setPlatform(Platform::Windows);
        const QVariantList menu = c.objectMenu();
        const QStringList ids = itemIds(QVariantMap{{QStringLiteral("items"), menu}});
        QCOMPARE(ids.mid(0, 3), (QStringList{"object.shape-points", "-", "edit.cut"}));
        QVERIFY(ids.contains(QStringLiteral("transform")));
        QVERIFY(ids.contains(QStringLiteral("align")));
        QVERIFY(ids.contains(QStringLiteral("arrange")));
        const auto submenuOf = [](const QVariantList& items, const QString& id) {
            for (const QVariant& item : items) {
                const QVariantMap map = item.toMap();
                if (map.value(QStringLiteral("submenu")).toBool() &&
                    map.value(QStringLiteral("id")).toString() == id) {
                    return map;
                }
            }
            return QVariantMap{};
        };
        const QVariantMap objectTransform = submenuOf(menu, QStringLiteral("transform"));
        QVERIFY(!objectTransform.isEmpty());
        QCOMPARE(objectTransform.value(QStringLiteral("items")).toList().size(), 13);
        QVariantMap editMenu;
        for (const QVariant& entry : c.menus()) {
            if (entry.toMap().value(QStringLiteral("id")).toString() == QLatin1String("edit")) {
                editMenu = entry.toMap();
            }
        }
        QCOMPARE(objectTransform.value(QStringLiteral("items")),
                 submenuOf(editMenu.value(QStringLiteral("items")).toList(), QStringLiteral("transform"))
                     .value(QStringLiteral("items")));
        // Sous Android, les entrées de la reconnaissance (Windows) disparaissent sans casser la liste.
        c.setPlatform(Platform::Android);
        QVERIFY(!itemIds(QVariantMap{{QStringLiteral("items"), c.objectMenu()}})
                     .contains(QStringLiteral("recognition.convert-to-text")));
    }

    void contextMenuWithSubmenu()
    {
        Commands c;
        c.setProfileDir(m_emptyProfile.path());
        c.setPlatform(Platform::Windows);
        const QVariantList menu = c.contextMenu();
        const QStringList ids = itemIds(QVariantMap{{QStringLiteral("items"), menu}});
        QCOMPARE(ids.mid(4, 2), (QStringList{QStringLiteral("-"), QStringLiteral("page-insert")}));
        const QVariantMap submenu = menu.at(5).toMap();
        QVERIFY(submenu.value(QStringLiteral("submenu")).toBool());
        QCOMPARE(itemIds(submenu),
                 (QStringList{QStringLiteral("page.insert-before"), QStringLiteral("page.insert-after")}));
    }

    void everyDeclaredIconExists()
    {
        Commands c;
        c.setProfileDir(m_emptyProfile.path());
        for (const QString& id : c.registry().ids()) {
            const QString url = c.iconUrl(id);
            if (!url.isEmpty()) {
                QVERIFY2(QFile::exists(url.mid(3)), qPrintable(url)); // « qrc:/… » → « :/… »
            }
        }
    }

    void profileLayoutOverridesResource()
    {
        QTemporaryDir profile;
        QVERIFY(writeFile(profile.filePath(QStringLiteral("layout.json")),
                          R"({"format":"actcraft-layout/1",
                              "menus":[{"id":"help","label":"Aide",
                                        "items":["-","file.quit","-","-","help.about","-"]}],
                              "shortcuts":{"page.next":"Space"}})"));
        Commands c;
        c.setProfileDir(profile.path());
        QCOMPARE(menuLabels(c), QStringList{QStringLiteral("Aide")});
        QCOMPARE(itemIds(c.menus().at(0)), (QStringList{QStringLiteral("file.quit"), QStringLiteral("-"),
                                                        QStringLiteral("help.about")}));
        QCOMPARE(c.shortcut(QStringLiteral("page.next")), QStringLiteral("Space"));
        bool found = false;
        for (const QVariant& s : c.shortcuts()) {
            const QVariantMap map = s.toMap();
            if (map.value(QStringLiteral("id")).toString() == QLatin1String("page.next")) {
                found = map.value(QStringLiteral("sequence")).toString() == QLatin1String("Space");
            }
        }
        QVERIFY(found);
        QVERIFY(c.palette().isEmpty());
    }

    void unreadableProfileLayoutFallsBackToResource()
    {
        QTemporaryDir profile;
        QVERIFY(writeFile(profile.filePath(QStringLiteral("layout.json")), QByteArray("{")));
        Commands c;
        c.setProfileDir(profile.path());
        QVERIFY(!c.errors().isEmpty());
        // Le repli est la disposition des ressources : ses six menus non vides depuis le socle des lots 3
        // et 7.
        QCOMPARE(menuLabels(c), (QStringList{QStringLiteral("Fichier"), QStringLiteral("Éditer"),
                                             QStringLiteral("Affichage"), QStringLiteral("Insérer"),
                                             QStringLiteral("Outils"), QStringLiteral("Aide")}));
    }

    // Un layoutChanged (commande masquée ailleurs) ne recrée pas les entrées d'un menu dont le contenu est
    // identique (L5-C25) ; le menu qui contenait la commande, lui, se met à jour.
    void layoutChangeKeepsUnchangedMenuEntries()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport ActCraft.Ui\nItem {\n"
                          "  property alias registre: commandes\n"
                          "  Commands { id: commandes }\n"
                          "  function menuSpec(id) {\n"
                          "    const m = commandes.menus\n"
                          "    for (let i = 0; i < m.length; ++i) if (m[i].id === id) return m[i]\n"
                          "    return { id: \"\", label: \"\", items: [] }\n"
                          "  }\n"
                          "  function entree(menu, i) { return menu.itemAt(i) }\n"
                          "  property alias fichier: fichierMenu\n"
                          "  property alias editer: editerMenu\n"
                          "  CommandMenu { id: fichierMenu; commands: commandes; spec: menuSpec(\"file\") }\n"
                          "  CommandMenu { id: editerMenu; commands: commandes; spec: menuSpec(\"edit\") }\n"
                          "}\n",
                          QUrl());
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        auto* c = qobject_cast<Commands*>(root->property("registre").value<QObject*>());
        QVERIFY(c);
        c->setProfileDir(m_emptyProfile.path());
        c->setPlatform(Platform::Windows);
        auto* fichier = root->property("fichier").value<QObject*>();
        auto* editer = root->property("editer").value<QObject*>();
        QVERIFY(fichier && editer);
        const auto itemAt = [&root](QObject* menu, int index) {
            QVariant item;
            QMetaObject::invokeMethod(root.get(), "entree", Q_RETURN_ARG(QVariant, item),
                                      Q_ARG(QVariant, QVariant::fromValue(menu)), Q_ARG(QVariant, index));
            return item.value<QObject*>();
        };
        const auto count = [](QObject* menu) { return menu->property("count").toInt(); };
        QObject* fichierFirst = itemAt(fichier, 0);
        QObject* editerFirst = itemAt(editer, 0);
        QVERIFY(fichierFirst && editerFirst);
        const int fichierCount = count(fichier);
        const int editerCount = count(editer);
        QCOMPARE(editerFirst->property("text").toString(), QStringLiteral("Annuler"));

        c->setHidden(QStringLiteral("edit.undo"), true); // commande du menu Éditer seulement

        QCOMPARE(count(fichier), fichierCount);
        QCOMPARE(itemAt(fichier, 0), fichierFirst); // même objet : rien n'a été recréé
        QCOMPARE(count(editer), editerCount - 1);
        QVERIFY(itemAt(editer, 0)->property("text").toString() != QStringLiteral("Annuler"));
    }

    void triggerReachesTheQmlHandler()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(
            "import QtQuick\nimport ActCraft.Ui\nItem {\n"
            "  property var recu: null\n"
            "  property QtObject registre: commandes\n"
            "  Commands { id: commandes }\n"
            "  Component.onCompleted: commandes.setHandler(\"pen.width\", function (w) { recu = w })\n"
            "}\n",
            QUrl());
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        auto* c = qobject_cast<Commands*>(root->property("registre").value<QObject*>());
        QVERIFY(c);
        QSignalSpy triggered(c, &Commands::triggered);
        QVERIFY(c->trigger(QStringLiteral("pen.width"), 8));
        QCOMPARE(root->property("recu").toDouble(), 8.0);
        QCOMPARE(triggered.count(), 1);
        QVERIFY(!c->trigger(QStringLiteral("pen.width"), QStringLiteral("épais")));
        QVERIFY(!c->trigger(QStringLiteral("page.next"))); // sans gestionnaire
        QVERIFY(!c->trigger(QStringLiteral("absent")));
        QCOMPARE(triggered.count(), 1);
        c->reload(); // les gestionnaires survivent au rechargement
        QVERIFY(c->trigger(QStringLiteral("pen.width"), 6));
        QCOMPARE(root->property("recu").toDouble(), 6.0);
    }

    // L'état coché (outil courant, plein écran) n'est pas dans les fichiers relus : gardé par reload().
    void reloadKeepsCheckedStates()
    {
        Commands c;
        c.setProfileDir(m_emptyProfile.path());
        c.setChecked(QStringLiteral("tool.pen"), true);
        c.setChecked(QStringLiteral("view.fullscreen"), true);
        c.reload();
        QVERIFY(c.isChecked(QStringLiteral("tool.pen")));
        QVERIFY(c.isChecked(QStringLiteral("view.fullscreen")));
        QVERIFY(!c.isChecked(QStringLiteral("view.fit")));
        c.setPlatform(Platform::Android); // recharge aussi
        QVERIFY(c.isChecked(QStringLiteral("tool.pen")));
    }

    void stateChangesBumpRevision()
    {
        Commands c;
        c.setProfileDir(m_emptyProfile.path());
        QSignalSpy state(&c, &Commands::stateChanged);
        const int before = c.revision();
        c.setChecked(QStringLiteral("view.fullscreen"), true);
        QVERIFY(c.isChecked(QStringLiteral("view.fullscreen")));
        QCOMPARE(c.revision(), before + 1);
        QCOMPARE(state.count(), 1);
        c.setEnabled(QStringLiteral("page.next"), false);
        QVERIFY(!c.isEnabled(QStringLiteral("page.next")));
        QCOMPARE(state.count(), 2);
        c.setChecked(QStringLiteral("view.fullscreen"), true); // inchangé : pas de signal
        QCOMPARE(state.count(), 2);
    }
};

QTEST_MAIN(TstUiCommands)
#include "tst_ui_commands.moc"
