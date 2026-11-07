import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Palette des enregistreurs (F-292, F-293) : enregistrer, pause, arrêter, abandonner ; mention de première
// utilisation. Porte l'instance `appRecorders` (les commandes `record.*` y sont liées).
Item {
    id: root
    objectName: "recorderPalette"
    property alias recorders: appRecorders
    property var commands: null
    property var document: null
    property var mediaInsert: null
    property var preferences: null
    readonly property bool active: appRecorders.paletteVisible && (appRecorders.recording || appRecorders.noticeVisible)
    visible: active
    z: 99999
    anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
    anchors.bottom: parent ? parent.bottom : undefined
    anchors.bottomMargin: 24
    width: column.width + 24
    height: column.height + 24

    Recorders {
        id: appRecorders
        objectName: "appRecorders"
        commands: root.commands
        document: root.document
        mediaInsert: root.mediaInsert
        preferences: root.preferences
    }

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: "#F2F4F7"
        border.color: "#B8BEC8"
    }
    Column {
        id: column
        anchors.centerIn: parent
        spacing: 8
        Text {
            objectName: "recorderNotice"
            visible: appRecorders.noticeVisible
            width: 340
            wrapMode: Text.WordWrap
            text: appRecorders.notice
            color: "#1F2430"
        }
        Button {
            visible: appRecorders.noticeVisible
            text: qsTr("Compris")
            onClicked: appRecorders.dismissNotice()
        }
        Row {
            spacing: 8
            visible: appRecorders.recording
            Button {
                objectName: "recorderPause"
                text: appRecorders.paused ? qsTr("Reprendre") : qsTr("Pause")
                onClicked: appRecorders.pause()
            }
            Button {
                objectName: "recorderStop"
                text: qsTr("Arrêter")
                onClicked: appRecorders.stop()
            }
            Button {
                objectName: "recorderCancel"
                text: qsTr("Abandonner")
                onClicked: appRecorders.cancel()
            }
            Button {
                objectName: "recorderHide"
                text: qsTr("Masquer")
                onClicked: appRecorders.hidePalette()
            }
        }
    }
}
