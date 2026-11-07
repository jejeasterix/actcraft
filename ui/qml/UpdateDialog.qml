import QtQuick
import ActCraft.Ui

// Talon L11 : boîte des mises à jour, remplie par L11-T22. Masquée, sans logique.
Item {
    id: dialog
    objectName: "updateDialog"
    required property Commands commands
    visible: false

    // Talon : le gestionnaire réel de `help.check-updates` remplace celui-ci (L11-T22). Gestionnaire (argument, user).
    Component.onCompleted: dialog.commands.setHandler("help.check-updates", function (argument, user) {})
}
