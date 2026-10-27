import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Roue des outils de bureau (F-080), L7-T31. Fenêtre sans cadre, transparente, toujours devant, créée SANS
// parent par DesktopAnnotation.qml (un parent la réduirait avec la fenêtre principale). Fermée : une icône
// ronde, déplaçable. Un clic la déploie : les entrées sont posées dans le sens horaire depuis le haut
// (DesktopController::wheelEntries) ; un anneau secondaire (caméra, autres outils) s'ouvre sur le cercle
// extérieur. Chaque entrée déclenche une commande nommée : la roue ne porte aucune action. Icône et roue sont
// dessinées ici.
Window {
    id: root
    objectName: "desktopToolsWheel"
    property var controller: null
    property var commands: null
    property bool expanded: false
    property int openRing: -1
    readonly property real iconSize: 72
    readonly property real wheelSize: 520
    readonly property var entries: root.controller ? root.controller.wheelEntries(110, 205) : []

    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"
    visible: root.controller ? root.controller.toolsShown : false
    width: root.expanded ? root.wheelSize : root.iconSize
    height: root.expanded ? root.wheelSize : root.iconSize
    title: qsTr("Outils de bureau")

    function setExpanded(value) {
        if (value === root.expanded) {
            return
        }
        // Le centre de l'icône reste en place.
        const delta = (root.wheelSize - root.iconSize) / 2
        root.x += value ? -delta : delta
        root.y += value ? -delta : delta
        root.expanded = value
        if (!value) {
            root.openRing = -1
        }
    }

    function trigger(command) {
        if (command === "") {
            return
        }
        root.setExpanded(false)
        if (root.commands) {
            root.commands.trigger(command)
        }
    }

    // Une fermeture de la roue (fenêtre principale revenue) la replie pour la fois suivante.
    onVisibleChanged: {
        if (!visible) {
            root.expanded = false
            root.openRing = -1
        }
    }

    // Icône : disque plein avec un anneau (dessin propre).
    Button {
        id: icon
        objectName: "desktopWheelIcon"
        x: (root.width - root.iconSize) / 2
        y: (root.height - root.iconSize) / 2
        width: root.iconSize
        height: root.iconSize
        text: qsTr("Outils")
        onClicked: root.setExpanded(!root.expanded)
        background: Rectangle {
            radius: width / 2
            color: root.expanded ? "#E6F2F2F2" : "#F21E4F8A"
            border.width: 2
            border.color: "#1E4F8A"
            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.5
                height: width
                radius: width / 2
                color: "transparent"
                border.width: 3
                border.color: root.expanded ? "#1E4F8A" : "white"
            }
        }
        contentItem: Item {}
        // Déplacer l'icône : glissement par le système ; le clic seul déploie.
        DragHandler {
            target: null
            enabled: !root.expanded
            onActiveChanged: {
                if (active) {
                    root.startSystemMove()
                }
            }
        }
    }

    Repeater {
        model: root.expanded ? root.entries : []

        Item {
            id: slot
            required property var modelData
            required property int index
            readonly property bool isRing: slot.modelData.ring.length > 0

            Button {
                objectName: slot.isRing ? "desktopWheelRing_" + slot.index
                                        : "desktopWheelEntry_" + slot.modelData.command
                width: 96
                height: 96
                x: root.width / 2 + slot.modelData.x - width / 2
                y: root.height / 2 + slot.modelData.y - height / 2
                text: slot.modelData.label
                highlighted: slot.isRing && root.openRing === slot.index
                onClicked: {
                    if (slot.isRing) {
                        root.openRing = root.openRing === slot.index ? -1 : slot.index
                    } else {
                        root.trigger(slot.modelData.command)
                    }
                }
                background: Rectangle {
                    radius: width / 2
                    color: parent.highlighted ? "#FFD7E6F7" : "#F2FFFFFF"
                    border.width: 2
                    border.color: "#1E4F8A"
                }
                contentItem: Text {
                    text: parent.text
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                    color: "#1B2733"
                }
            }

            Repeater {
                model: slot.isRing && root.openRing === slot.index ? slot.modelData.ring : []

                Button {
                    required property var modelData
                    objectName: "desktopWheelEntry_" + modelData.command
                    width: 84
                    height: 84
                    x: root.width / 2 + modelData.x - width / 2
                    y: root.height / 2 + modelData.y - height / 2
                    text: modelData.label
                    onClicked: root.trigger(modelData.command)
                    background: Rectangle {
                        radius: width / 2
                        color: "#F2FFFFFF"
                        border.width: 2
                        border.color: "#4B7FB8"
                    }
                    contentItem: Text {
                        text: parent.text
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 12
                        color: "#1B2733"
                    }
                }
            }
        }
    }
}
