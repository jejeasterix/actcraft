import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Sous-menu « Mode du toucher » du menu Affichage : quatre choix exclusifs, cochés selon le mode du contrôleur
// (`source`). La commande touch.mode jouée sans argument (l'entrée du menu) ouvre ce menu ; un choix rejoue la
// commande avec son identifiant. Aucune autre logique ici.
Item {
    id: host
    objectName: "touchModeMenu"
    // Pas « menu » : MenuItem porte sa propre propriété `menu`.
    required property Commands commands
    property QtObject source: null // TouchModeController
    readonly property string mode: source ? source.mode : "full"
    readonly property bool opened: popup.opened
    visible: false

    function open() {
        // Sous la ligne supérieure, près du menu Affichage.
        popup.popup(130, parent ? parent.height : 0)
    }
    function choose(id) {
        host.commands.trigger("touch.mode", id)
    }
    function checkedMode() {
        for (let i = 0; i < choices.count; ++i) {
            const item = choices.itemAt(i)
            if (item && item.checked)
                return item.modelData.id
        }
        return ""
    }

    Connections {
        target: host.source
        function onMenuRequested() { host.open() }
    }

    Menu {
        id: popup
        title: qsTr("Mode du toucher")
        Repeater {
            id: choices
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
                checked: host.mode === modelData.id
                // L'exclusivité vient du mode : un clic rend la coche à la liaison.
                onToggled: checked = Qt.binding(function () { return host.mode === modelData.id })
                onTriggered: host.choose(modelData.id)
            }
        }
    }
}
