import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qt.labs.qmlmodels
import ActCraft.Ui

// Menu construit depuis la disposition (actions/data/layout.json) : une entrée par commande disponible ; libellé,
// raccourci et coche lus dans le registre ; un clic déclenche la commande par son nom. Les entrées sont créées par
// un Instantiator (jamais par createObject sans parent : elles survivaient au menu et lisaient un registre disparu).
// Un élément `submenu` crée un Menu enfant (un CommandMenu) par le même Instantiator : un DelegateChooser choisit le
// délégué d'après la colonne `kind` d'un ListModel, car un tableau JavaScript n'expose que `modelData`.
Menu {
    id: menu
    required property Commands commands
    property var spec: ({ id: "", label: "", items: [] })
    readonly property bool isCommandMenu: true
    // Documents récents (L6-T24) : liste de FileActions (`fileActions.recents`), liée par le menu Fichier de Main.qml.
    property var recents: null
    title: commands.translated(spec.label)
    // Au doigt (L11-T12, D11-7) : rangées de la taille d'une cible (appLayout.targetSize, relayée par Theme.tailleBouton,
    // comme CommandEntry) et sous-menus ouverts par un appui seulement (pas de survol sans pointeur).
    readonly property real rowHeight: Theme.tailleBouton
    delegate: MenuItem {
        implicitHeight: menu.rowHeight
        hoverEnabled: !Theme.touchUi
    }
    // Largeur : la plus large des entrées (libellé + raccourci). Le style de base se règle sur un fond de 200 px et
    // ne suit pas la largeur des contenus : libellés tronqués, raccourcis sur les libellés.
    implicitWidth: {
        let widest = 0
        let pad = 0
        for (let i = 0; i < menu.count; ++i) {
            const it = menu.itemAt(i)
            if (!it || !it.contentItem)
                continue
            widest = Math.max(widest, it.contentItem.implicitWidth)
            pad = Math.max(pad, it.leftPadding + it.rightPadding)
        }
        return Math.max(implicitBackgroundWidth + leftInset + rightInset, widest + pad + leftPadding + rightPadding)
    }

    ListModel {
        id: entries
    }
    function syncEntries() {
        const items = menu.spec.items || []
        const rows = items.map(it => ({ kind: it.submenu === true ? "submenu" : "item", payload: JSON.stringify(it) }))
        // Même contenu (layoutChanged émis pour une autre commande) : rien à recréer.
        if (rows.length === entries.count && rows.every((row, i) => entries.get(i).payload === row.payload))
            return
        entries.clear()
        for (const row of rows)
            entries.append(row)
    }
    onSpecChanged: syncEntries()
    Component.onCompleted: syncEntries()

    // Sous-menu dynamique « Documents récents », inséré après `file.open` du menu Fichier tant que la liste n'est
    // pas vide ; chaque entrée rejoue `file.open <url>`.
    property bool recentShown: false
    function syncRecent() {
        const wanted = menu.spec.id === "file" && menu.recents && menu.recents.count > 0
        if (menu.recentShown && !wanted) {
            menu.removeMenu(recentMenu)
            menu.recentShown = false
        }
        if (!wanted)
            return
        if (!menu.recentShown) {
            for (let i = 0; i < menu.count; ++i) {
                const it = menu.itemAt(i)
                if (it && it.objectName === "menuItem-file.open") {
                    menu.insertMenu(i + 1, recentMenu)
                    menu.recentShown = true
                    break
                }
            }
        }
    }
    onRecentsChanged: Qt.callLater(menu.syncRecent)
    Connections {
        target: menu.recents
        function onChanged() { Qt.callLater(menu.syncRecent) }
    }
    property Menu recentMenu: Menu {
        objectName: "recentDocumentsMenu"
        title: qsTr("Documents récents")
        Instantiator {
            model: menu.recents ? menu.recents.entries : []
            delegate: MenuItem {
                required property var modelData
                objectName: "recentDocument-" + modelData.name
                text: modelData.name
                onTriggered: if (menu.commands) menu.commands.trigger("file.open", modelData.url)
            }
            onObjectAdded: (index, object) => recentMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => recentMenu.removeItem(object)
        }
    }

    Instantiator {
        model: entries
        delegate: DelegateChooser {
            role: "kind"
            DelegateChoice {
                roleValue: "submenu"
                CommandSubmenu {
                    required property string payload
                    commands: menu ? menu.commands : null
                    spec: JSON.parse(payload)
                }
            }
            DelegateChoice {
                roleValue: "item"
                CommandEntry {
                    commands: menu ? menu.commands : null
                }
            }
        }
        onObjectAdded: (index, object) => {
            object.isCommandMenu === true ? menu.insertMenu(index, object) : menu.insertItem(index, object)
            Qt.callLater(menu.syncRecent)
        }
        onObjectRemoved: (index, object) => {
            object.isCommandMenu === true ? menu.removeMenu(object) : menu.removeItem(object)
            menu.recentShown = false // l'indice du sous-menu a bougé : il est reposé
            Qt.callLater(menu.syncRecent)
        }
    }
}
