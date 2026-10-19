import QtQuick
import QtQuick.Controls
import ActCraft.Ui

// Réglette de translucidité, de 0 (opaque) à 255 (invisible) : ouverte par object.translucency sans argument.
// Chaque mouvement déclenche object.translucency <valeur> ; une étape d'annulation par fusion (L3-T4).
Popup {
    id: popup

    required property Tools tools

    parent: Overlay.overlay
    x: Math.round((parent.width - width) / 2)
    y: Math.round(parent.height - height - 2 * Theme.espace4)
    width: 280
    padding: Theme.espace3
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        color: Theme.palette
        radius: Theme.rayon
        border.width: 1
        border.color: Theme.contour
    }

    contentItem: Column {
        spacing: Theme.espace2
        Text {
            text: popup.tools.commands ? popup.tools.commands.label("object.translucency") : ""
            color: Theme.texte
            font.pixelSize: Theme.taillePolice
        }
        Slider {
            id: slider
            objectName: "translucencySlider"
            width: parent.width
            from: 0
            to: 255
            stepSize: 1
            value: 0
            implicitHeight: Theme.tailleBouton
            Accessible.name: popup.tools.commands ? popup.tools.commands.label("object.translucency") : ""
            onMoved: popup.tools.commands.trigger("object.translucency", Math.round(value))
            background: Rectangle {
                x: slider.leftPadding
                y: slider.topPadding + (slider.availableHeight - height) / 2
                width: slider.availableWidth
                height: 6
                radius: 3
                color: Theme.surfaceSurvol
                Rectangle {
                    width: slider.visualPosition * parent.width
                    height: parent.height
                    radius: 3
                    color: Theme.surfaceActive
                }
            }
            handle: Rectangle {
                x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
                y: slider.topPadding + (slider.availableHeight - height) / 2
                width: Theme.taillePastille
                height: width
                radius: width / 2
                color: Theme.texte
                border.width: 2
                border.color: Theme.surfaceActive
            }
        }
    }
}
