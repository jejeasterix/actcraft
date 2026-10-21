import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Réglage du compte à rebours (F-290, L7-T16) : boîte flottante (FloatingPanel : couverture de la page). Durée
// « hh:mm:ss » (champ countdownField), action de fin (aucune, page suivante, page précédente), répétition,
// « Réinitialiser » et « Démarrer ». Entrée dans le champ ou sur la liste des actions valide : la durée est
// prise, le compte à rebours part et la boîte se ferme. Le clavier suffit : Tab passe du champ à la liste,
// haut et bas changent d'action. Ouverte par ClockPanel (open()).
FloatingPanel {
    id: root
    property ClockController controller: null
    property bool invalid: false

    title: qsTr("Compte à rebours")
    visible: false
    width: 320
    height: Theme.tailleBouton + column.implicitHeight + 2 * Theme.espace2

    // Remplit la boîte depuis le contrôleur, l'ouvre et met le champ au clavier.
    function open() {
        root.invalid = false
        if (root.controller) {
            field.text = root.controller.countdownText
            endAction.currentIndex = Math.max(0, endAction.indexOfValue(root.controller.endAction))
        }
        root.visible = true
        field.forceActiveFocus()
        field.selectAll()
    }

    // Prend la durée et l'action de fin, lance le compte à rebours, ferme la boîte. Faux (la boîte reste) si la
    // durée est invalide.
    function apply() {
        if (!root.controller) {
            return false
        }
        if (!root.controller.applyCountdownText(field.text)) {
            root.invalid = true
            field.forceActiveFocus()
            return false
        }
        root.controller.endAction = endAction.currentValue
        root.controller.startCountdown()
        root.close()
        return true
    }

    Column {
        id: column
        width: parent ? parent.width : 0
        spacing: Theme.espace2

        Row {
            spacing: Theme.espace2
            Label {
                width: 150
                height: Theme.tailleBouton
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Durée (hh:mm:ss)")
                textFormat: Text.PlainText
                color: Theme.texte
                font.pixelSize: Theme.taillePolice
            }
            TextField {
                id: field
                objectName: "countdownField"
                width: column.width - 150 - Theme.espace2
                height: Theme.tailleBouton
                color: Theme.texte
                font.pixelSize: Theme.taillePoliceTitre
                horizontalAlignment: Text.AlignHCenter
                selectByMouse: true
                inputMethodHints: Qt.ImhPreferNumbers
                validator: RegularExpressionValidator { regularExpression: /^[0-9:]{0,8}$/ }
                background: Rectangle {
                    radius: Theme.rayon
                    color: Theme.surface
                    border.color: root.invalid ? "#E57373" : (field.activeFocus ? Theme.surfaceActive : Theme.contour)
                    border.width: field.activeFocus ? 2 : 1
                }
                KeyNavigation.tab: endAction
                onTextEdited: root.invalid = false
                onAccepted: root.apply()
                Keys.onEscapePressed: root.close()
            }
        }

        Label {
            objectName: "countdownError"
            visible: root.invalid
            width: parent.width
            text: qsTr("Durée invalide : saisir heures, minutes et secondes, par exemple 00:05:00.")
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
            color: "#E57373"
            font.pixelSize: Theme.taillePolice
        }

        Row {
            spacing: Theme.espace2
            Label {
                width: 150
                height: Theme.tailleBouton
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Action de fin")
                textFormat: Text.PlainText
                color: Theme.texte
                font.pixelSize: Theme.taillePolice
            }
            ComboBox {
                id: endAction
                objectName: "countdownEndAction"
                width: column.width - 150 - Theme.espace2
                height: Theme.tailleBouton
                model: root.controller ? root.controller.endActions : []
                textRole: "label"
                valueRole: "command"
                font.pixelSize: Theme.taillePolice
                KeyNavigation.tab: repeat
                KeyNavigation.backtab: field
                Keys.onReturnPressed: root.apply()
                Keys.onEnterPressed: root.apply()
                Keys.onEscapePressed: root.close()
            }
        }

        CheckBox {
            id: repeat
            objectName: "countdownRepeat"
            width: parent.width
            height: Theme.tailleBouton
            text: qsTr("Répéter")
            checked: root.controller ? root.controller.repeat : false
            font.pixelSize: Theme.taillePolice
            palette.windowText: Theme.texte
            KeyNavigation.tab: start
            KeyNavigation.backtab: endAction
            onToggled: if (root.controller) root.controller.repeat = checked
        }

        Row {
            spacing: Theme.espace2
            Button {
                objectName: "countdownReset"
                width: (column.width - Theme.espace2) / 2
                height: Theme.tailleBouton
                text: qsTr("Réinitialiser")
                onClicked: {
                    if (root.controller && root.controller.applyCountdownText(field.text)) {
                        root.invalid = false
                        root.controller.resetCountdown()
                    } else {
                        root.invalid = true
                    }
                }
            }
            Button {
                id: start
                objectName: "countdownStart"
                width: (column.width - Theme.espace2) / 2
                height: Theme.tailleBouton
                text: qsTr("Démarrer")
                KeyNavigation.backtab: repeat
                onClicked: root.apply()
            }
        }
    }
}
