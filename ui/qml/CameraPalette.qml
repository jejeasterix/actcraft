import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Palette de la caméra (F-275), L7-T28 : élément de la fenêtre de sélection, jamais de la fenêtre principale.
// pageArea: null : elle n'est pas une couverture de la vue principale (un élément d'une autre fenêtre ; ses
// coordonnées de scène n'y désignent rien). Destinations : page affichée, nouvelle page, presse-papiers (les
// ressources arrivent au lot 6, H7-3) ; pour une zone, gauche, haut, largeur et hauteur en pixels de l'écran.
FloatingPanel {
    id: palette
    property CameraController controller: null
    readonly property bool areaMode: palette.controller !== null
                                     && palette.controller.mode === CameraController.AreaMode
    readonly property rect area: palette.controller ? palette.controller.selection : Qt.rect(0, 0, 0, 0)
    readonly property bool ready: palette.controller !== null && palette.controller.ready

    title: qsTr("Caméra")
    pageArea: null
    width: 440
    height: Theme.tailleBouton + rows.implicitHeight + 2 * Theme.espace2

    function setArea(left, top, w, h) {
        if (palette.controller)
            palette.controller.selection = Qt.rect(left, top, w, h)
    }

    function deliver(destination) {
        if (palette.controller)
            palette.controller.deliver(destination)
    }

    Column {
        id: rows
        width: parent ? parent.width : 0
        spacing: Theme.espace2

        Grid {
            visible: palette.areaMode
            columns: 4
            columnSpacing: Theme.espace2
            rowSpacing: Theme.espace1
            verticalItemAlignment: Grid.AlignVCenter

            Label {
                text: qsTr("Gauche")
                color: Theme.texte
            }
            SpinBox {
                objectName: "cameraZoneLeft"
                editable: true
                from: 0
                to: 100000
                value: Math.round(palette.area.x)
                onValueModified: palette.setArea(value, palette.area.y, palette.area.width, palette.area.height)
            }
            Label {
                text: qsTr("Haut")
                color: Theme.texte
            }
            SpinBox {
                objectName: "cameraZoneTop"
                editable: true
                from: 0
                to: 100000
                value: Math.round(palette.area.y)
                onValueModified: palette.setArea(palette.area.x, value, palette.area.width, palette.area.height)
            }
            Label {
                text: qsTr("Largeur")
                color: Theme.texte
            }
            SpinBox {
                objectName: "cameraZoneWidth"
                editable: true
                from: 1
                to: 100000
                value: Math.round(palette.area.width)
                onValueModified: palette.setArea(palette.area.x, palette.area.y, value, palette.area.height)
            }
            Label {
                text: qsTr("Hauteur")
                color: Theme.texte
            }
            SpinBox {
                objectName: "cameraZoneHeight"
                editable: true
                from: 1
                to: 100000
                value: Math.round(palette.area.height)
                onValueModified: palette.setArea(palette.area.x, palette.area.y, palette.area.width, value)
            }
        }

        Flow {
            width: rows.width
            spacing: Theme.espace2

            Button {
                objectName: "cameraToCurrentPage"
                text: qsTr("Page actuelle")
                enabled: palette.ready
                onClicked: palette.deliver(CameraController.CurrentPage)
            }
            Button {
                objectName: "cameraToNewPage"
                text: qsTr("Nouvelle page")
                enabled: palette.ready
                onClicked: palette.deliver(CameraController.NewPage)
            }
            Button {
                objectName: "cameraToClipboard"
                text: qsTr("Presse-papiers")
                enabled: palette.ready
                onClicked: palette.deliver(CameraController.Clipboard)
            }
        }
    }
}
