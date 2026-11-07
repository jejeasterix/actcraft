import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Sous-menu « Mode du toucher » du menu Affichage : quatre choix exclusifs, cochés selon `mode` (celui du
// contrôleur du mode du toucher). Un choix joue la commande touch.mode ; aucune autre logique ici.
Item {
    id: menu
    objectName: "touchModeMenu"
    required property Commands commands
    property string mode: "full"
    visible: false

    function open() {
        popup.popup()
    }

    Menu {
        id: popup
        title: qsTr("Mode du toucher")
        Repeater {
            model: [
                { id: "ignore", label: qsTr("Ignorer le toucher") },
                { id: "pen", label: qsTr("Le doigt écrit") },
                { id: "select", label: qsTr("Le doigt sélectionne") },
                { id: "full", label: qsTr("Gestion complète") }
            ]
            MenuItem {
                required property var modelData
                objectName: "touchMode-" + modelData.id
                text: modelData.label
                checkable: true
                autoExclusive: true
                checked: menu.mode === modelData.id
                onTriggered: menu.commands.trigger("touch.mode", modelData.id)
            }
        }
    }
}
