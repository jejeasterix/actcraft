import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Dés (F-285, L7-T14) : panneau flottant (FloatingPanel : couverture de la page, titre, croix, déplaçable). Il
// s'ouvre par la commande tool.dice et se ferme par sa croix (le lancer en cours s'arrête). Nombre de dés (1 à 5)
// et durée du lancer (3 à 11 s) sont ceux du contrôleur, qui les mémorise ; « Écrire sur la page » pose le total
// en texte sur la page affichée. Les faces sont dessinées ici, aucune image.
FloatingPanel {
    id: root
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property QtObject controller: diceController

    title: qsTr("Dés")
    visible: false
    width: 300
    height: Theme.tailleBouton + column.implicitHeight + 2 * Theme.espace2
    x: parent ? Math.max(0, parent.width - width - 40) : 0
    y: 90

    DiceController {
        id: diceController
        objectName: "dice"
        documentController: root.documentController
    }

    // Une face : les points du dé, sur une grille de trois par trois.
    component DieFace: Rectangle {
        id: face
        property int value: 1
        readonly property var pips: [[4], [0, 8], [0, 4, 8], [0, 2, 6, 8], [0, 2, 4, 6, 8], [0, 2, 3, 5, 6, 8]]
        width: 48
        height: 48
        radius: Theme.rayon
        color: Theme.surface
        border.color: Theme.contour
        Repeater {
            model: 9
            Rectangle {
                required property int index
                width: 8
                height: 8
                radius: 4
                x: 4 + (index % 3) * 16
                y: 4 + Math.floor(index / 3) * 16
                color: Theme.texte
                visible: face.value >= 1 && face.value <= 6 && face.pips[face.value - 1].indexOf(index) >= 0
            }
        }
    }

    // Bouton du panneau. Le stylet n'atteint pas toujours le clic du bouton : un TapHandler propre au stylet
    // l'active aussi ; un anti-rebond évite le double déclenchement si les deux voies arrivent.
    component PanelButton: Button {
        id: pb
        property double last: 0
        signal activated()
        function fire() {
            const now = Date.now()
            if (now - pb.last < 300)
                return
            pb.last = now
            pb.activated()
        }
        onClicked: pb.fire()
        TapHandler {
            acceptedDevices: PointerDevice.Stylus
            onTapped: pb.fire()
        }
    }

    // Un réglage entier borné, avec ses deux boutons.
    component Stepper: Row {
        id: stepper
        property string key
        property string caption
        property int value: 0
        property int from: 0
        property int to: 0
        signal picked(int value)
        spacing: Theme.espace1
        Label {
            width: 150
            height: Theme.tailleBouton
            verticalAlignment: Text.AlignVCenter
            text: stepper.caption
            textFormat: Text.PlainText
            color: Theme.texte
            font.pixelSize: Theme.taillePolice
        }
        PanelButton {
            objectName: stepper.key + "Down"
            width: Theme.tailleBouton
            height: Theme.tailleBouton
            text: "−"
            enabled: stepper.value > stepper.from
            Accessible.name: qsTr("Diminuer")
            onActivated: stepper.picked(stepper.value - 1)
        }
        Label {
            objectName: stepper.key + "Value"
            width: 30
            height: Theme.tailleBouton
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: stepper.value
            color: Theme.texte
            font.pixelSize: Theme.taillePolice
        }
        PanelButton {
            objectName: stepper.key + "Up"
            width: Theme.tailleBouton
            height: Theme.tailleBouton
            text: "+"
            enabled: stepper.value < stepper.to
            Accessible.name: qsTr("Augmenter")
            onActivated: stepper.picked(stepper.value + 1)
        }
    }

    Column {
        id: column
        width: parent ? parent.width : 0
        spacing: Theme.espace2

        Item {
            width: parent.width
            height: 48
            Row {
                spacing: Theme.espace1
                Repeater {
                    model: diceController.faces
                    DieFace {
                        required property var modelData
                        value: modelData
                    }
                }
            }
        }

        Stepper {
            key: "diceCount"
            caption: qsTr("Nombre de dés")
            value: diceController.count
            from: diceController.minCount
            to: diceController.maxCount
            onPicked: v => diceController.count = v
        }

        Stepper {
            key: "diceSpeed"
            caption: qsTr("Durée du lancer (s)")
            value: diceController.speed
            from: diceController.minSpeed
            to: diceController.maxSpeed
            onPicked: v => diceController.speed = v
        }

        Row {
            spacing: Theme.espace2
            PanelButton {
                objectName: "diceRollButton"
                width: (column.width - Theme.espace2) / 2
                height: Theme.tailleBouton
                text: diceController.rolling ? qsTr("Arrêter") : qsTr("Lancer")
                onActivated: diceController.rolling ? diceController.skip() : diceController.roll()
            }
            PanelButton {
                objectName: "diceWriteButton"
                width: (column.width - Theme.espace2) / 2
                height: Theme.tailleBouton
                text: qsTr("Écrire sur la page")
                enabled: diceController.canWrite
                onActivated: diceController.writeTotal()
            }
        }

        Label {
            objectName: "diceTotal"
            width: parent.width
            height: Theme.tailleBouton
            verticalAlignment: Text.AlignVCenter
            text: diceController.canWrite ? qsTr("Total : %1").arg(diceController.total) : ""
            textFormat: Text.PlainText
            color: Theme.texte
            font.pixelSize: Theme.taillePoliceTitre
        }
    }

    // Fermer le panneau arrête le lancer en cours.
    onClosed: diceController.skip()

    function registerCommands() {
        if (!root.commands) {
            return
        }
        root.commands.setHandler("tool.dice", function () {
            root.visible = true
        })
    }

    onCommandsChanged: registerCommands()
    Component.onCompleted: registerCommands()
}
