import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Sous-menu d'un CommandMenu (« Éditer ▸ Effacer »). Un seul niveau : il ne crée que des entrées de commande ou des
// séparateurs, jamais d'autre sous-menu (un composant QML ne peut pas s'instancier lui-même).
Menu {
    id: menu
    required property Commands commands
    property var spec: ({ id: "", label: "", items: [] })
    readonly property bool isCommandMenu: true
    title: spec.label

    Instantiator {
        model: menu.spec.items || []
        delegate: CommandEntry {
            required property var modelData
            commands: menu.commands
            payload: JSON.stringify(modelData)
        }
        onObjectAdded: (index, object) => menu.insertItem(index, object)
        onObjectRemoved: (index, object) => menu.removeItem(object)
    }
}
