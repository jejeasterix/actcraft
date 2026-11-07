import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Window

// Indicateur d'enregistrement (F-292, F-293) : visible de l'appui jusqu'à la fermeture du fichier, au-dessus
// de tout. Une pastille rouge qui bat, le type d'enregistrement et la durée, et le bouton Arrêter (toujours à
// portée, sous le verrou de classe aussi).
//
// Son : une pastille dans la fenêtre principale. Écran : la fenêtre principale réduite ou couverte ne le
// montrerait pas, et il entrerait dans l'image ; il vit alors dans une fenêtre à part, toujours au-dessus, posée
// hors de la zone enregistrée (Recorders.indicatorSpot). Qt n'offre pas d'exclusion d'une fenêtre de la capture :
// en plein écran, la fenêtre entre dans l'image.
Item {
    id: root
    objectName: "recordingIndicator"
    property var recorders: null
    readonly property bool active: recorders ? recorders.indicatorVisible : false
    readonly property bool separate: recorders ? recorders.separateIndicator : false
    readonly property bool paused: recorders ? recorders.paused : false
    readonly property bool running: active && !paused
    // Temps enregistré, pauses exclues : les secondes écoulées avant une pause sont gardées, la fraction de
    // seconde en cours aussi (un minuteur relancé à chaque reprise en perdait jusqu'à une).
    property real accumulatedMs: 0
    property real resumedAt: 0
    property real tick: 0
    readonly property int elapsedSeconds: Math.floor((accumulatedMs + (running ? Math.max(0, tick - resumedAt) : 0)) / 1000)

    anchors.fill: parent
    visible: active
    z: 100000

    onActiveChanged: if (active) accumulatedMs = 0
    onRunningChanged: {
        const now = Date.now()
        if (running) {
            resumedAt = now
            tick = now
        } else {
            accumulatedMs += now - resumedAt
        }
    }
    Timer {
        interval: 250
        repeat: true
        running: root.running
        onTriggered: root.tick = Date.now()
    }

    Component {
        id: badge
        Rectangle {
            width: Math.max(row.width + 24, 240)
            height: 36
            radius: 18
            color: "#CC1F2430"
            border.color: "#FFFFFF"
            border.width: 1
            Accessible.role: Accessible.StaticText
            Accessible.name: label.text

            Row {
                id: row
                anchors.centerIn: parent
                spacing: 8
                Rectangle {
                    width: 14
                    height: 14
                    radius: 7
                    anchors.verticalCenter: parent.verticalCenter
                    color: root.paused ? "#F2C94C" : "#E5322D"
                    SequentialAnimation on opacity {
                        running: root.running
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.35; duration: 500 }
                        NumberAnimation { to: 1.0; duration: 500 }
                    }
                }
                Text {
                    id: label
                    anchors.verticalCenter: parent.verticalCenter
                    color: "white"
                    font.pixelSize: 15
                    text: {
                        if (!root.recorders)
                            return ""
                        const what = root.recorders.kind === "screen" ? qsTr("Écran") : qsTr("Son")
                        const state = root.paused ? qsTr("en pause") : qsTr("enregistré")
                        const minutes = Math.floor(root.elapsedSeconds / 60)
                        const seconds = root.elapsedSeconds % 60
                        return what + " " + state + " " + minutes + ":" + (seconds < 10 ? "0" : "") + seconds
                    }
                }
                // L'arrêt reste toujours à portée, palette masquée ou non (et sous le verrou de classe).
                Button {
                    objectName: "indicatorStop"
                    anchors.verticalCenter: parent.verticalCenter
                    height: 26
                    text: qsTr("Arrêter")
                    onClicked: root.recorders.stop()
                }
            }
        }
    }

    // Enregistrement du son : la pastille de la fenêtre principale.
    Loader {
        objectName: "recordingIndicatorBadge"
        active: root.active && !root.separate
        sourceComponent: badge
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 12
    }

    // Enregistrement de l'écran : une fenêtre à part, au-dessus de tout, posée hors de la zone enregistrée.
    Window {
        id: floating
        objectName: "recordingIndicatorWindow"
        readonly property var primary: Qt.application.screens.length > 0 ? Qt.application.screens[0] : null
        visible: root.active && root.separate
        flags: Qt.WindowStaysOnTopHint | Qt.FramelessWindowHint | Qt.Tool
        color: "transparent"
        width: content.item ? content.item.width : 240
        height: 36
        screen: primary
        onVisibleChanged: if (visible) place()
        function place() {
            if (!primary || !root.recorders)
                return
            const g = primary.geometry
            const spot = root.recorders.indicatorSpot(Qt.rect(g.x, g.y, g.width, g.height), Qt.size(width, height))
            x = spot.x
            y = spot.y
        }
        Loader {
            id: content
            active: floating.visible
            sourceComponent: badge
        }
    }
}
