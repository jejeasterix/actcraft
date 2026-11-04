import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import ActCraft.Ui

// Grille étendue de 6 × 6 couleurs (F-135), avec le copieur de couleur (« color.pick », porté par ColorLoupe.qml)
// et la boîte de couleur. Toute couleur choisie devient la couleur du stylo et entre dans la palette (« color.add »).
Item {
    id: root
    objectName: "extendedColors"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    visible: false

    // Même nom que l'instance de ColorSwatches.qml : celle-ci vit dans un Loader et la fenêtre ne la retrouve pas
    // par findChild (lectures des scénarios) ; les deux lisent le même profil.
    ColorPalette {
        id: pal
        objectName: "appColors"
        profile: root.profile
        baseColors: root.commands ? root.commands.colors : []
    }

    Popup {
        id: popup
        objectName: "extendedColorsPopup"
        parent: Overlay.overlay
        padding: Theme.espace2
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        enter: null
        exit: null
        background: Rectangle {
            color: Theme.palette
            radius: 6
            border.width: 1
            border.color: Theme.contour
        }
        contentItem: Column {
            spacing: Theme.espace2
            Grid {
                columns: 6
                spacing: Theme.espace1
                Repeater {
                    model: pal.extended
                    delegate: Rectangle {
                        required property string modelData
                        required property int index
                        objectName: "extended-" + index
                        width: Theme.taillePastille + 4
                        height: width
                        radius: 3
                        color: modelData
                        border.width: 1
                        border.color: Theme.contour
                        Accessible.role: Accessible.Button
                        Accessible.name: qsTr("Couleur %1").arg(modelData)
                        TapHandler {
                            onTapped: {
                                popup.close()
                                root.commands.trigger("color.add", modelData)
                            }
                        }
                    }
                }
            }
            Row {
                spacing: Theme.espace2
                Button {
                    objectName: "colorPickButton"
                    text: qsTr("Copieur")
                    onClicked: {
                        popup.close()
                        root.commands.trigger("color.pick")
                    }
                }
                Button {
                    objectName: "colorDialogButton"
                    text: qsTr("Boîte de couleur")
                    onClicked: {
                        popup.close()
                        root.commands.trigger("color.dialog")
                    }
                }
            }
        }
    }

    ColorDialog {
        id: dialog
        title: qsTr("Choisir une couleur")
        onAccepted: root.commands.trigger("color.add", selectedColor.toString())
    }

    Component.onCompleted: {
        root.commands.setHandler("color.more", function (spot) {
            popup.open()
            // Dans la fenêtre : la palette est au bord droit.
            popup.x = Math.max(0, Math.min(spot && spot.x !== undefined ? spot.x : 0, popup.parent.width - popup.width))
            popup.y = Math.max(0, Math.min(spot && spot.y !== undefined ? spot.y : 0, popup.parent.height - popup.height))
        })
        root.commands.setHandler("color.dialog", function () { dialog.open() })
        // Couleur du stylo, puis palette : sans doublon (ColorPalette.add).
        root.commands.setHandler("color.add", function (color) {
            if (!color)
                return
            root.commands.trigger("pen.color", color)
            pal.add(color)
        })
    }
}
