import QtQuick
import ActCraft.Ui

// Talon L11 : deuxième palette du second utilisateur (bord gauche), remplie par L11-T9. Masquée tant que le mode à
// deux utilisateurs est éteint ; aucune logique ici.
Item {
    id: palette
    objectName: "secondPalette"
    required property Commands commands
    implicitWidth: Theme.largeurPalette

    // Talon : le gestionnaire réel de `users.dual` remplace celui-ci (L11-T9). Gestionnaire (argument, user).
    Component.onCompleted: palette.commands.setHandler("users.dual", function (argument, user) {})
}
