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
    title: spec.label

    ListModel {
        id: entries
    }
    function syncEntries() {
        entries.clear()
        const items = menu.spec.items || []
        for (let i = 0; i < items.length; ++i) {
            entries.append({ kind: items[i].submenu === true ? "submenu" : "item", payload: JSON.stringify(items[i]) })
        }
    }
    onSpecChanged: syncEntries()
    Component.onCompleted: syncEntries()

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
        onObjectAdded: (index, object) => object.isCommandMenu === true ? menu.insertMenu(index, object) : menu.insertItem(index, object)
        onObjectRemoved: (index, object) => object.isCommandMenu === true ? menu.removeMenu(object) : menu.removeItem(object)
    }
}
