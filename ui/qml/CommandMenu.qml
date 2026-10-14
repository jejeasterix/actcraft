import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Menu construit depuis la disposition (actions/data/layout.json) : une entrée par commande disponible ; libellé,
// raccourci et coche lus dans le registre ; un clic déclenche la commande par son nom.
Menu {
    id: menu
    required property Commands commands
    property var spec: ({ id: "", label: "", items: [] })
    title: spec.label

    function rebuild() {
        while (menu.count > 0)
            menu.removeItem(menu.itemAt(0))
        const items = spec.items || []
        for (let i = 0; i < items.length; ++i) {
            if (items[i].separator)
                menu.addItem(separatorComponent.createObject(null))
            else
                menu.addItem(entryComponent.createObject(null, {
                    commandId: items[i].id,
                    commandCheckable: items[i].checkable === true
                }))
        }
    }

    onSpecChanged: rebuild()
    Component.onCompleted: rebuild()

    Component {
        id: separatorComponent
        MenuSeparator {}
    }

    Component {
        id: entryComponent
        MenuItem {
            id: entry
            property string commandId
            property bool commandCheckable: false
            objectName: "menuItem-" + commandId
            text: menu.commands.label(commandId)
            enabled: menu.commands.revision >= 0 && menu.commands.isEnabled(commandId)
            checkable: commandCheckable
            checked: menu.commands.revision >= 0 && menu.commands.isChecked(commandId)
            // Le clic bascule la coche localement : on rend la main au registre.
            onToggled: checked = Qt.binding(function () {
                return menu.commands.revision >= 0 && menu.commands.isChecked(entry.commandId)
            })
            onTriggered: menu.commands.trigger(commandId)
            contentItem: RowLayout {
                spacing: Theme.espace3
                Label {
                    text: entry.text
                    Layout.fillWidth: true
                    opacity: entry.enabled ? 1 : 0.5
                }
                Label {
                    text: menu.commands.shortcut(entry.commandId)
                    opacity: 0.6
                }
            }
        }
    }
}
