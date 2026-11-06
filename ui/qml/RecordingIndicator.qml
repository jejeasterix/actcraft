import QtQuick
import QtQuick.Controls.Basic

// Indicateur d'enregistrement (F-292, F-293) : visible de l'appui jusqu'à la fermeture du fichier, au-dessus
// de tout. Une pastille rouge qui bat, le type d'enregistrement et la durée.
Rectangle {
    id: root
    objectName: "recordingIndicator"
    property var recorders: null
    readonly property bool active: recorders ? recorders.indicatorVisible : false
    property int elapsedSeconds: 0
    visible: active
    z: 100000
    anchors.top: parent ? parent.top : undefined
    anchors.right: parent ? parent.right : undefined
    anchors.margins: 12
    width: row.width + 24
    height: 36
    radius: 18
    color: "#CC1F2430"
    border.color: "#FFFFFF"
    border.width: 1
    Accessible.role: Accessible.StaticText
    Accessible.name: label.text

    Timer {
        interval: 1000
        repeat: true
        running: root.active && !(root.recorders && root.recorders.paused)
        onTriggered: root.elapsedSeconds += 1
    }
    onActiveChanged: elapsedSeconds = 0

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 8
        Rectangle {
            width: 14
            height: 14
            radius: 7
            anchors.verticalCenter: parent.verticalCenter
            color: root.recorders && root.recorders.paused ? "#F2C94C" : "#E5322D"
            SequentialAnimation on opacity {
                running: root.active && !(root.recorders && root.recorders.paused)
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
                const state = root.recorders.paused ? qsTr("en pause") : qsTr("enregistré")
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
