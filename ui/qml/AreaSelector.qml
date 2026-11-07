import QtQuick
import QtQuick.Window

// Tracé de la zone d'écran à enregistrer (F-292, « plein écran ou zone ») : un calque sur la fenêtre, un
// rectangle tiré au glisser ; Échap ou clic droit abandonne, et rien ne démarre. La zone part en coordonnées
// logiques de l'écran, avec le rapport de pixels et l'origine de l'écran : Recorders la convertit en pixels de
// l'image capturée (Recorders.pixelArea).
FocusScope {
    id: root
    objectName: "areaSelector"
    property var recorders: null
    readonly property bool active: recorders ? recorders.selectingArea : false
    // L'enregistreur capture l'écran principal : une zone tracée sur un autre écran ne le désignerait pas.
    readonly property bool onPrimaryScreen: Qt.application.screens.length === 0
                                            || Screen.name === Qt.application.screens[0].name
    property bool dragging: false
    property real startX: 0
    property real startY: 0
    property real currentX: 0
    property real currentY: 0
    readonly property real bandX: Math.min(startX, currentX)
    readonly property real bandY: Math.min(startY, currentY)
    readonly property real bandWidth: Math.abs(currentX - startX)
    readonly property real bandHeight: Math.abs(currentY - startY)

    anchors.fill: parent
    visible: active
    focus: active
    z: 100001
    onActiveChanged: {
        dragging = false
        if (active)
            forceActiveFocus()
    }
    Keys.onEscapePressed: root.recorders.cancelAreaSelection()

    Rectangle {
        anchors.fill: parent
        color: "#66000000"
    }
    Rectangle {
        objectName: "areaSelectorBand"
        visible: root.dragging
        x: root.bandX
        y: root.bandY
        width: root.bandWidth
        height: root.bandHeight
        color: "#33FFFFFF"
        border.color: "#FFFFFF"
        border.width: 2
    }
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 24
        width: hint.width + 32
        height: hint.height + 16
        radius: 8
        color: "#E61F2430"
        Text {
            id: hint
            anchors.centerIn: parent
            color: "white"
            font.pixelSize: 15
            text: root.onPrimaryScreen
                  ? qsTr("Tracez la zone de l'écran à enregistrer (Échap pour annuler)")
                  : qsTr("Placez la fenêtre sur l'écran principal pour tracer la zone (Échap pour annuler)")
        }
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: root.onPrimaryScreen ? Qt.CrossCursor : Qt.ArrowCursor
        onPressed: mouse => {
            if (mouse.button === Qt.RightButton) {
                root.recorders.cancelAreaSelection()
                return
            }
            if (!root.onPrimaryScreen)
                return
            root.startX = root.currentX = mouse.x
            root.startY = root.currentY = mouse.y
            root.dragging = true
        }
        onPositionChanged: mouse => {
            if (!root.dragging)
                return
            root.currentX = Math.max(0, Math.min(width, mouse.x))
            root.currentY = Math.max(0, Math.min(height, mouse.y))
        }
        onReleased: mouse => {
            if (!root.dragging)
                return
            root.dragging = false
            // Coordonnées logiques de l'écran (mapToGlobal), rapport de pixels et origine de cet écran.
            const corner = root.mapToGlobal(root.bandX, root.bandY)
            root.recorders.finishAreaSelection(corner.x, corner.y, root.bandWidth, root.bandHeight,
                                               Screen.devicePixelRatio, Screen.virtualX, Screen.virtualY)
        }
    }
}
