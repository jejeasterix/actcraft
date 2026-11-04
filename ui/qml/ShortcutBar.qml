import QtQuick
import ActCraft.Ui

// Troisième colonne de la palette (F-140, F-141) : outils lancés hors palette, puis boutons d'objets.
Item {
    id: root
    objectName: "shortcutBar"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    readonly property alias model: appShortcuts
    readonly property bool hasButtons: appShortcuts.buttons.length > 0
    implicitWidth: column.implicitWidth
    implicitHeight: column.implicitHeight

    ShortcutBarModel {
        id: appShortcuts
        objectName: "appShortcuts"
        commands: root.commands
        profile: root.profile
        document: root.documentController
    }

    Column {
        id: column
        spacing: Theme.espace1
        Repeater {
            model: appShortcuts.buttons
            delegate: Loader {
                id: cell
                required property var modelData
                sourceComponent: cell.modelData.kind === "tool" ? toolButton : objectButton
            }
        }
    }

    Component {
        id: toolButton
        CommandButton {
            commands: root.commands
            commandId: cell.modelData.id
        }
    }

    Component {
        id: objectButton
        Rectangle {
            width: 40
            height: 40
            radius: 4
            color: "transparent"
            border.color: "gray"
            Text {
                anchors.fill: parent
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                text: cell.modelData.label
            }
            MouseArea {
                anchors.fill: parent
                onClicked: appShortcuts.insert(cell.modelData.id)
            }
        }
    }

    // Le glisser de la sélection n'existe pas encore : l'ajout et l'insertion passent par la commande
    // `shortcutbar.insert` (argument : identifiant du bouton).
    Component.onCompleted: {
        root.commands.setHandler("shortcutbar.insert", function (id) {
            if (id && id.length > 0)
                appShortcuts.insert(id)
        })
    }
}
