import QtQuick
import ActCraft.Ui

// Corbeille (F-050) sous la palette : un objet sélectionné qu'on y dépose est supprimé (l'outil lit sa zone dans
// tools.trashArea, en coordonnées de scène) ; une flèche rouge s'allume quand l'objet tenu est au-dessus
// (tools.trashHover) ; un toucher supprime la sélection, par la commande nommée « edit.delete ».
Item {
    id: can
    required property Tools tools
    required property Commands commands
    // Identifiant lu depuis une propriété : l'AOT refuse l'argument littéral (voir ToolPalette.qml).
    readonly property string deleteId: "edit.delete"
    readonly property color flecheRouge: "#FF6B6B" // plus de 3:1 sur la palette

    implicitHeight: Theme.tailleBouton + Theme.espace2 * 2
    height: implicitHeight

    // Zone en coordonnées de scène, recalculée à chaque changement de géométrie (position, taille, fenêtre).
    function publish() {
        if (!can.visible || can.width <= 0 || can.height <= 0) {
            can.tools.trashArea = Qt.rect(0, 0, 0, 0)
            return
        }
        can.tools.trashArea = can.mapToItem(null, 0, 0, can.width, can.height)
    }
    onXChanged: publish()
    onYChanged: publish()
    onWidthChanged: publish()
    onHeightChanged: publish()
    onVisibleChanged: publish()
    Component.onCompleted: publish()
    Connections {
        target: can.Window.window
        function onWidthChanged() { can.publish() }
        function onHeightChanged() { can.publish() }
    }

    Rectangle {
        id: well
        anchors.fill: parent
        anchors.margins: Theme.espace2
        radius: Theme.rayon
        color: can.tools.trashHover ? Theme.surfaceSurvol : Theme.surface
        border.width: 1
        border.color: can.tools.trashHover ? can.flecheRouge : Theme.contour
        Accessible.role: Accessible.Button
        Accessible.name: can.commands.label(can.deleteId)

        Image {
            id: bin
            objectName: "trashIcon"
            anchors.centerIn: parent
            width: Theme.tailleIcone
            height: Theme.tailleIcone
            sourceSize.width: Theme.tailleIcone
            sourceSize.height: Theme.tailleIcone
            source: "qrc:/qt/qml/ActCraft/Ui/icons/corbeille.svg"
            enabled: false
        }

        // Flèche rouge vers la corbeille : tige et deux barres de pointe.
        Item {
            id: arrow
            objectName: "trashArrow"
            visible: can.tools.trashHover
            enabled: false
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 2
            width: 16
            height: 12
            Rectangle {
                x: 7
                y: 0
                width: 3
                height: 12
                color: can.flecheRouge
            }
            Rectangle {
                x: 1
                y: 6
                width: 9
                height: 3
                radius: 1
                rotation: 45
                transformOrigin: Item.Right
                color: can.flecheRouge
            }
            Rectangle {
                x: 7
                y: 6
                width: 9
                height: 3
                radius: 1
                rotation: -45
                transformOrigin: Item.Left
                color: can.flecheRouge
            }
        }

        TapHandler {
            id: tap
            onTapped: can.commands.trigger(can.deleteId)
        }
    }
}