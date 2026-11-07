import QtQuick
import ActCraft.Ui

// Talon L11 : bandeau de reprise après une interruption, rempli par L11-T13. Masqué, sans logique.
Item {
    id: banner
    objectName: "recoveryBanner"
    required property Commands commands
    visible: false

    // Talon : le gestionnaire réel de `recovery.discard` remplace celui-ci (L11-T13). Gestionnaire (argument, user).
    Component.onCompleted: banner.commands.setHandler("recovery.discard", function (argument, user) {})
}
