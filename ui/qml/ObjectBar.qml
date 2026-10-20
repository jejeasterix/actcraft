import QtQuick
import ActCraft.Ui

// Barre d'objet (F-521) : boutons carrés aux icônes des commandes, placés par l'outil (centres en coordonnées de
// page, au-dessus du cadre, au-dessous faute de place, à droite d'un cadre plus haut que large). Les clics sont
// désignés par l'outil (hitSelection) : aucun gestionnaire ici.
Item {
    id: bar

    required property Tools tools
    required property Commands commands
    required property var overlay // SelectionOverlay : place() convertit page → surcouche

    enabled: false

    Repeater {
        model: bar.tools.selectionFrame.bar
        delegate: Rectangle {
            id: button
            required property var modelData
            readonly property point at: bar.overlay.place(Qt.point(modelData.x, modelData.y))
            x: at.x - width / 2
            y: at.y - height / 2
            width: 24
            height: 24
            radius: 4
            color: Theme.surface
            border.width: 1
            border.color: Theme.contour
            Accessible.name: bar.commands.label(modelData.command)

            Image {
                anchors.centerIn: parent
                width: 16
                height: 16
                sourceSize.width: 16
                sourceSize.height: 16
                source: button.modelData.icon
            }
        }
    }
}
