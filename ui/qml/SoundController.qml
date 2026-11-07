import QtQuick

// L8-T25 (S25) : lecteur de sons (commande media.sound-controller) ; montre le son de la piste 1 et sa position
// qui avance. Il s'ouvre aussi quand un son démarre. Ne prend l'entrée que sur son bouton d'arrêt.
Rectangle {
    id: controller
    objectName: "soundController"
    property var commands: null
    property var items: null // MediaItems de la fenêtre
    property var view: null // vue de page : le lecteur prend l'appui du stylet (patron de DragDropActions)
    readonly property bool playing: items ? items.soundPlaying : false
    readonly property real position: items ? items.soundPosition : 0
    // État demandé, distinct de la visibilité effective (qui dépend aussi du parent) : la bascule le lit.
    property bool shown: false
    visible: shown
    width: 240
    height: 40
    radius: 6
    color: "#2B2F36"
    anchors.right: parent ? parent.right : undefined
    anchors.bottom: parent ? parent.bottom : undefined
    anchors.margins: 16
    z: 50

    onViewChanged: if (view) view.addInputCover(controller)

    function toggle() {
        controller.shown = !controller.shown
    }
    function timeText(ms) {
        const s = Math.floor(ms / 1000)
        return Math.floor(s / 60) + ":" + String(s % 60).padStart(2, "0")
    }
    Connections {
        target: controller.items
        ignoreUnknownSignals: true
        function onSoundStarted() { controller.shown = true }
    }
    Text {
        objectName: "soundControllerPosition"
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        color: "#E8EAED"
        text: (controller.playing ? "▶ " : "■ ") + controller.timeText(controller.position)
              + (controller.items && controller.items.soundName ? "  " + controller.items.soundName : "")
        elide: Text.ElideRight
        width: parent.width - 50
    }
    MouseArea {
        objectName: "soundControllerStop"
        anchors.right: parent.right
        width: 40
        height: parent.height
        onClicked: if (controller.items) controller.items.stopSound()
        Text { anchors.centerIn: parent; text: "■"; color: "#E8EAED" }
    }
    Component.onCompleted: {
        if (view)
            view.addInputCover(controller)
        if (!commands)
            return
        commands.setHandler("media.sound-controller", function (arg) { controller.toggle() })
    }
}
