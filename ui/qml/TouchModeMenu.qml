import QtQuick
import ActCraft.Ui

// Talon L11 : sous-menu « Mode du toucher » du menu Affichage, rempli par L11-T6. Aucune logique ici.
Item {
    id: menu
    objectName: "touchModeMenu"
    required property Commands commands
    visible: false

    // Talon : le gestionnaire réel de `touch.mode` remplace celui-ci (L11-T6). Gestionnaire (argument, user).
    Component.onCompleted: menu.commands.setHandler("touch.mode", function (argument, user) {})
}
