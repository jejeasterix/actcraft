import QtQuick
import ActCraft.Ui

// Pastilles de couleur (F-133, F-134) : 4 colonnes ; un toucher choisit la couleur du stylo par la commande
// « pen.color » ; la couleur courante est encadrée.
Grid {
    id: grid
    objectName: "colorSwatches"
    required property Commands commands
    required property PageView view
    columns: 4
    spacing: Theme.espace1

    Repeater {
        model: grid.commands.colors
        delegate: Rectangle {
            id: swatch
            required property string modelData
            required property int index
            readonly property bool current: Qt.colorEqual(grid.view.inkColor, modelData)
            objectName: "swatch-" + index
            width: Theme.taillePastille
            height: Theme.taillePastille
            radius: 3
            color: modelData
            // Bord « contour » : les pastilles sombres se détachent de la palette. Choisie : cadre blanc doublé à
            // l'intérieur d'un filet de la couleur de la palette, visible aussi sur la pastille blanche (L1-C5).
            border.width: current ? 2 : 1
            border.color: current ? Theme.cadreSelection : Theme.contour
            scale: tap.pressed ? 0.9 : 1 // appui visible au doigt
            Accessible.role: Accessible.RadioButton
            Accessible.name: qsTr("Couleur %1").arg(modelData)
            Accessible.checked: current
            Accessible.onPressAction: grid.commands.trigger("pen.color", swatch.modelData)
            Rectangle {
                visible: swatch.current
                anchors.fill: parent
                anchors.margins: 2
                radius: 1
                color: "transparent"
                border.width: 1
                border.color: Theme.palette
            }
            TapHandler {
                id: tap
                margin: Theme.espace1 / 2 // cible contiguë : le pas de la grille (24 px), pas seulement la pastille
                onTapped: grid.commands.trigger("pen.color", swatch.modelData)
            }
        }
    }
}
