import QtQuick
import QtQuick.Controls.Basic
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
    property bool horizontal: false
    readonly property alias model: appShortcuts
    readonly property bool hasButtons: appShortcuts.buttons.length > 0
    readonly property alias launchers: launchers
    readonly property int launcherCount: launchers.entries.length
    readonly property string editId: "launcher.edit"
    readonly property string runId: "launcher.run"
    // E3 : bouton d'édition en bas, absent si la palette est réduite ou la barre masquée.
    readonly property bool editVisible: root.profile ? !(root.profile.paletteCollapsed || root.profile.shortcutsHidden) : true
    readonly property bool editShown: root.editVisible && root.commands.isAvailable(root.editId)
    // Au moins un bouton à montrer : sans lui, la palette ne réserve aucune place à la barre.
    readonly property bool hasContent: root.hasButtons || root.launcherCount > 0 || root.editShown
    implicitWidth: column.implicitWidth
    implicitHeight: column.implicitHeight

    ShortcutBarModel {
        id: appShortcuts
        objectName: "appShortcuts"
        commands: root.commands
        profile: root.profile
        document: root.documentController
        tools: root.tools
    }

    // Raccourcis vers des programmes (F-142) : lus du profil, lancés par la commande launcher.run (une seule
    // boîte de confirmation, celle de LaunchersPage).
    Launchers {
        id: launchers
        objectName: "barLaunchers"
        profile: root.profile
    }

    // Une colonne, ou une rangée comme les groupes de la palette quand elle est horizontale (ancrée en haut ou
    // en bas : une seule rangée de boutons tient dans sa hauteur).
    Grid {
        id: column
        spacing: Theme.espace1
        columns: root.horizontal ? -1 : 1
        rows: root.horizontal ? 1 : -1
        Repeater {
            model: appShortcuts.buttons
            delegate: Loader {
                id: cell
                required property var modelData
                sourceComponent: cell.modelData.kind === "tool" ? toolButton : objectButton
            }
        }
        Repeater {
            model: launchers.entries
            delegate: launcherButton
        }
        CommandButton {
            objectName: "launcherEditButton"
            commands: root.commands
            commandId: root.editId
            visible: root.editShown
        }
    }

    Component {
        id: launcherButton
        ToolButton {
            id: launcherDelegate
            required property int index
            required property var modelData
            property string toolTip: modelData.toolTip
            objectName: "launcherButton" + index
            implicitWidth: Theme.tailleBouton
            implicitHeight: Theme.tailleBouton
            enabled: root.commands.revision >= 0 && root.commands.isEnabled(root.runId)
            focusPolicy: Qt.NoFocus
            display: AbstractButton.IconOnly
            icon.source: root.commands.iconUrl(root.editId)
            icon.width: Theme.tailleIcone
            icon.height: Theme.tailleIcone
            Accessible.name: toolTip
            ToolTip.visible: hovered
            ToolTip.delay: 600
            ToolTip.text: toolTip
            onClicked: root.commands.trigger(root.runId, modelData.id)
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

    // Le lot 5 n'émet aucun signal de glisser de la sélection : l'ajout passe par la commande
    // `shortcutbar.add-selection` (menu d'objet, gestionnaire posé par le modèle), l'insertion par
    // `shortcutbar.insert` (argument : identifiant du bouton).
    Component.onCompleted: {
        root.commands.setHandler("shortcutbar.insert", function (id) {
            if (id && id.length > 0)
                appShortcuts.insert(id)
        })
    }
}
