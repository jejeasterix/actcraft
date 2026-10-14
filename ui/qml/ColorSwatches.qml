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
            readonly property bool current: Qt.colorEqual(grid.view.penColor, modelData)
            objectName: "swatch-" + index
            width: Theme.taillePastille
            height: Theme.taillePastille
            radius: 3
            color: modelData
            border.width: current ? 2 : 1
            border.color: current ? Theme.cadreSelection : Theme.separateur
            TapHandler {
                onTapped: grid.commands.trigger("pen.color", swatch.modelData)
            }
        }
    }
}
