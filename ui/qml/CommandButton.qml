import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Bouton relié à une commande nommée (DA1) : icône, info-bulle avec raccourci, état actif et coché lus dans le
// registre ; un clic déclenche la commande par son nom.
ToolButton {
    id: button
    required property Commands commands
    required property string commandId
    property var argument: undefined

    objectName: "button-" + commandId
    implicitWidth: Theme.tailleBouton
    implicitHeight: Theme.tailleBouton
    enabled: commands.revision >= 0 && commands.isEnabled(commandId)
    checked: commands.revision >= 0 && commands.isChecked(commandId)
    display: AbstractButton.IconOnly
    focusPolicy: Qt.NoFocus // les raccourcis restent à la fenêtre : un bouton n'avale aucune touche
    icon.source: commands.iconUrl(commandId)
    icon.width: Theme.tailleIcone
    icon.height: Theme.tailleIcone
    icon.color: "transparent" // garder la couleur dessinée de l'icône
    ToolTip.visible: hovered
    ToolTip.delay: 600
    ToolTip.text: commands.shortcut(commandId) === ""
                  ? commands.label(commandId)
                  : qsTr("%1 (%2)").arg(commands.label(commandId)).arg(commands.shortcut(commandId))

    background: Rectangle {
        radius: Theme.rayon
        color: button.checked ? Theme.surfaceActive : button.hovered ? Theme.surfaceSurvol : Theme.surface
    }

    onClicked: {
        if (button.argument === undefined)
            commands.trigger(commandId)
        else
            commands.trigger(commandId, button.argument)
    }
}
