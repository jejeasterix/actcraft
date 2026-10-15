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
    // Icônes d'une seule couleur, teintées par l'état (L1-C5) : appuyé ou coché sur l'accent, indisponible estompé.
    // Au doigt il n'y a pas de survol : l'appui seul doit se voir.
    readonly property bool onAccent: button.checked || button.down
    icon.color: !button.enabled ? Theme.texteDesactive : button.onAccent ? Theme.texteSurActive : Theme.texte
    Accessible.name: commands.label(commandId)
    ToolTip.visible: hovered
    ToolTip.delay: 600
    ToolTip.text: commands.shortcut(commandId) === ""
                  ? commands.label(commandId)
                  : qsTr("%1 (%2)").arg(commands.label(commandId)).arg(commands.shortcut(commandId))

    background: Rectangle {
        radius: Theme.rayon
        color: button.onAccent ? Theme.surfaceActive : button.hovered ? Theme.surfaceSurvol : Theme.surface
    }

    onClicked: {
        if (button.argument === undefined)
            commands.trigger(commandId)
        else
            commands.trigger(commandId, button.argument)
    }
}
