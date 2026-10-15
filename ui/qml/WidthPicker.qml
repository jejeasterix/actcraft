import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Épaisseur du stylo (F-137, F-138) : quatre préréglages ronds, réglette de 1 à 100, échantillon de la largeur
// et de la couleur courantes. Tout passe par la commande « pen.width ».
ColumnLayout {
    id: picker
    objectName: "widthPicker"
    required property Commands commands
    required property PageView view
    spacing: Theme.espace1

    Row {
        spacing: Theme.espace1
        Repeater {
            model: picker.commands.penWidths
            delegate: Rectangle {
                id: preset
                required property real modelData
                required property int index
                objectName: "widthPreset-" + index
                width: 21
                height: 21
                radius: 4
                color: picker.view.penWidth === modelData ? Theme.surfaceActive : Theme.surface
                Rectangle {
                    anchors.centerIn: parent
                    width: Math.min(17, 3 + preset.modelData * 1.5)
                    height: width
                    radius: width / 2
                    color: Theme.texte
                }
                TapHandler {
                    onTapped: picker.commands.trigger("pen.width", preset.modelData)
                }
            }
        }
    }

    Slider {
        id: slider
        objectName: "widthSlider"
        Layout.fillWidth: true
        from: 1
        to: 100
        stepSize: 1
        value: picker.view.penWidth
        onMoved: picker.commands.trigger("pen.width", value)
        // Le glissement casse la liaison : on la rend au stylo au relâché.
        onPressedChanged: if (!pressed) value = Qt.binding(() => picker.view.penWidth)
    }

    Item {
        Layout.fillWidth: true
        implicitHeight: 40
        Rectangle {
            objectName: "widthIndicatorSample"
            anchors.centerIn: parent
            width: Math.min(36, Math.max(2, picker.view.penWidth))
            height: width
            radius: width / 2
            color: picker.view.penColor
            border.width: 1
            border.color: Theme.separateur
        }
    }
}
