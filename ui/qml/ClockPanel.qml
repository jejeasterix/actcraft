import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Horloge, minuterie et compte à rebours (F-290, L7-T16) : panneau flottant (FloatingPanel : couverture de la
// page, titre, croix, déplaçable). Il s'ouvre par la commande tool.clock ; fermé, il arrête compte à rebours
// et minuterie. Cadran analogique, affichage numérique ou les deux ; barre de boutons (affichage, source, pause,
// réglage du compte à rebours) affichable ou masquable ; les quatre coins agrandissent le panneau en gardant ses
// proportions. Les aiguilles suivent les secondes du contrôleur (une mise à jour par seconde, aucune
// animation) ; le panneau ne redessine pas le cache de la page. Le réglage du compte à rebours est la boîte
// CountdownDialog.
FloatingPanel {
    id: root
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property QtObject controller: clockController
    property bool barVisible: true
    readonly property real minWidth: 220
    readonly property real maxScale: 2.5
    readonly property bool showFace: clockController.mode !== ClockController.Digital
    readonly property bool showDigital: clockController.mode !== ClockController.Analog

    title: qsTr("Horloge")
    visible: false
    width: 280
    height: 340
    x: parent ? Math.max(0, parent.width - 280 - 40) : 0
    y: 90

    ClockController {
        id: clockController
        objectName: "clock"
        commands: root.commands
        active: root.visible
    }

    CountdownDialog {
        id: countdownDialog
        objectName: "countdownDialog"
        parent: root.parent
        z: root.z + 1
        controller: clockController
        pageArea: root.pageArea
        x: parent ? Math.max(0, (parent.width - width) / 2) : 0
        y: 120
    }

    // Une aiguille : tourne autour du centre du cadran.
    component Hand: Item {
        id: hand
        property real angle: 0
        property real length: 50
        property real thickness: 3
        property color tint: Theme.texte
        anchors.centerIn: parent
        width: 0
        height: 0
        rotation: hand.angle
        Rectangle {
            x: -hand.thickness / 2
            y: -hand.length
            width: hand.thickness
            height: hand.length
            radius: hand.thickness / 2
            color: hand.tint
        }
    }

    Item {
        id: content
        anchors.fill: parent

        Item {
            id: display
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: root.barVisible ? bar.top : parent.bottom
            anchors.bottomMargin: root.barVisible ? Theme.espace2 : 0

            Item {
                id: face
                objectName: "clockFace"
                visible: root.showFace
                width: Math.max(0, Math.min(display.width, root.showDigital ? display.height * 0.68 : display.height))
                height: width
                anchors.horizontalCenter: parent.horizontalCenter
                y: root.showDigital ? 0 : (display.height - height) / 2

                Rectangle {
                    anchors.fill: parent
                    radius: width / 2
                    color: Theme.surface
                    border.color: Theme.contour
                    border.width: 2
                }
                Repeater {
                    model: 12
                    Item {
                        required property int index
                        anchors.centerIn: parent
                        width: 0
                        height: 0
                        rotation: index * 30
                        Rectangle {
                            x: -1
                            y: -face.width / 2 + 5
                            width: 2
                            height: index % 3 === 0 ? 10 : 5
                            color: Theme.texte
                        }
                    }
                }
                Hand {
                    objectName: "clockHourHand"
                    angle: clockController.hourAngle
                    length: face.width * 0.26
                    thickness: 4
                }
                Hand {
                    objectName: "clockMinuteHand"
                    angle: clockController.minuteAngle
                    length: face.width * 0.38
                    thickness: 3
                }
                Hand {
                    objectName: "clockSecondHand"
                    angle: clockController.secondAngle
                    length: face.width * 0.4
                    thickness: 1
                    tint: Theme.surfaceActive
                }
            }

            Column {
                id: digital
                objectName: "clockDigitalBlock"
                visible: root.showDigital
                width: parent.width
                y: root.showFace ? face.height + Theme.espace1 : (display.height - height) / 2
                Label {
                    objectName: "clockDigital"
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: clockController.displayText
                    textFormat: Text.PlainText
                    color: Theme.texte
                    font.pixelSize: Math.max(18, Math.round(root.width / 8))
                    font.bold: true
                }
                Label {
                    objectName: "clockSourceLabel"
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: clockController.source === ClockController.Stopwatch ? qsTr("Minuterie")
                          : clockController.source === ClockController.Countdown ? qsTr("Compte à rebours")
                          : qsTr("Heure")
                    textFormat: Text.PlainText
                    color: Theme.texteDiscret
                    font.pixelSize: Theme.taillePolice
                }
            }
        }

        Row {
            id: bar
            objectName: "clockBar"
            visible: root.barVisible
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            spacing: Theme.espace1
            readonly property real cell: (width - 3 * spacing) / 4

            Button {
                objectName: "clockModeButton"
                width: bar.cell
                height: Theme.tailleBouton
                text: qsTr("Cadran")
                font.pixelSize: 12
                leftPadding: 2
                rightPadding: 2
                onClicked: clockController.cycleMode()
            }
            Button {
                objectName: "clockSourceButton"
                width: bar.cell
                height: Theme.tailleBouton
                text: qsTr("Source")
                font.pixelSize: 12
                leftPadding: 2
                rightPadding: 2
                onClicked: clockController.cycleSource()
            }
            Button {
                objectName: "clockPauseButton"
                width: bar.cell
                height: Theme.tailleBouton
                text: clockController.paused ? qsTr("Reprise") : qsTr("Pause")
                font.pixelSize: 12
                leftPadding: 2
                rightPadding: 2
                onClicked: clockController.togglePause()
            }
            Button {
                objectName: "clockSettingsButton"
                width: bar.cell
                height: Theme.tailleBouton
                text: qsTr("Réglage")
                font.pixelSize: 12
                leftPadding: 2
                rightPadding: 2
                onClicked: countdownDialog.open()
            }
        }

        // Affiche ou masque la barre de boutons.
        Button {
            objectName: "clockBarToggle"
            anchors.top: parent.top
            anchors.right: parent.right
            width: 28
            height: 28
            flat: true
            text: root.barVisible ? "▾" : "▴"
            padding: 0
            Accessible.name: root.barVisible ? qsTr("Masquer la barre") : qsTr("Afficher la barre")
            onClicked: root.barVisible = !root.barVisible
        }
    }

    // Poignée de coin : agrandit ou réduit le panneau en gardant ses proportions (projection du glissé sur la
    // diagonale). `sx` et `sy` valent +1 pour un coin droit ou bas, -1 pour un coin gauche ou haut. Le coin
    // opposé reste fixe et l'échelle est bornée par la place qu'il laisse dans le parent : le panneau ne sort
    // jamais de la couche (barre de titre et croix restent atteignables, comme au déplacement par la barre).
    component CornerGrip: Item {
        id: grip
        property int sx: 1
        property int sy: 1
        width: 18
        height: 18
        z: 10
        DragHandler {
            id: drag
            target: null
            property real w0: 0
            property real h0: 0
            property real x0: 0
            property real y0: 0
            onActiveChanged: {
                if (active) {
                    w0 = root.width
                    h0 = root.height
                    x0 = root.x
                    y0 = root.y
                }
            }
            onCentroidChanged: {
                if (!active)
                    return
                const dx = centroid.scenePosition.x - centroid.scenePressPosition.x
                const dy = centroid.scenePosition.y - centroid.scenePressPosition.y
                const raw = 1 + (grip.sx * dx * w0 + grip.sy * dy * h0) / (w0 * w0 + h0 * h0)
                const roomX = grip.sx < 0 ? x0 + w0 : (root.parent ? root.parent.width - x0 : w0 * root.maxScale)
                const roomY = grip.sy < 0 ? y0 + h0 : (root.parent ? root.parent.height - y0 : h0 * root.maxScale)
                const fit = Math.min(roomX / w0, roomY / h0)
                const s = Math.max(root.minWidth / w0, Math.min(root.maxScale, fit, raw))
                root.width = w0 * s
                root.height = h0 * s
                root.x = grip.sx < 0 ? x0 + w0 - root.width : x0
                root.y = grip.sy < 0 ? y0 + h0 - root.height : y0
            }
        }
        // Repère visuel : deux traits en L dans l'angle.
        Rectangle {
            width: 10
            height: 2
            color: Theme.contour
            x: grip.sx > 0 ? parent.width - width - 3 : 3
            y: grip.sy > 0 ? parent.height - 3 - height : 3
        }
        Rectangle {
            width: 2
            height: 10
            color: Theme.contour
            x: grip.sx > 0 ? parent.width - 3 - width : 3
            y: grip.sy > 0 ? parent.height - height - 3 : 3
        }
    }

    CornerGrip {
        objectName: "clockCornerTopLeft"
        parent: root
        sx: -1
        sy: -1
        x: 0
        y: 0
    }
    CornerGrip {
        objectName: "clockCornerTopRight"
        parent: root
        sx: 1
        sy: -1
        x: root.width - width
        y: 0
    }
    CornerGrip {
        objectName: "clockCornerBottomLeft"
        parent: root
        sx: -1
        sy: 1
        x: 0
        y: root.height - height
    }
    CornerGrip {
        objectName: "clockCornerBottomRight"
        parent: root
        sx: 1
        sy: 1
        x: root.width - width
        y: root.height - height
    }

    // Fermer le panneau arrête le compte à rebours et la minuterie, et ferme la boîte de réglage.
    onClosed: {
        clockController.stop()
        countdownDialog.visible = false
    }

    function registerCommands() {
        if (!root.commands) {
            return
        }
        root.commands.setHandler("tool.clock", function () {
            root.commands.requestMainWindow("tool.clock")
            root.visible = true
        })
    }

    onCommandsChanged: registerCommands()
    Component.onCompleted: registerCommands()
}
