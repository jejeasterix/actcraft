import QtQuick
import ActCraft.Ui

// Plateau des icônes de glisser-déposer (F-892, D8-11), à côté du panneau des navigateurs quand son onglet
// « Actions » est ouvert. Une icône lâchée sur un objet de la page lui donne l'action correspondante
// (DragDropActionsModel.dropAt, une étape) ; la surimpression de l'objet est dessinée par ActionOverlays.
// Le plateau prend l'entrée là où il est (view.addInputCover) : un appui sur une icône ne va pas à l'outil.
Rectangle {
    id: tray
    objectName: "dragDropActions"
    property var model: null // DragDropActionsModel (objectName « appDragDrop »)
    property Item panel: null // panneau des navigateurs
    property var view: null // vue de page
    readonly property bool shown: panel !== null && panel.visible && panel.current === "actions"
    readonly property int cell: 40
    readonly property int columns: 3
    readonly property int margin: 8
    readonly property int headerHeight: 24

    visible: shown
    z: 20
    color: Theme.palette
    border.color: Theme.contour
    border.width: 1
    radius: Theme.rayon
    width: columns * cell + margin * 2
    height: headerHeight + Math.ceil(grid.count / columns) * cell + margin * 2
    Accessible.name: qsTr("Glisser-déposer")

    // Au bord du panneau, du côté de la page (dépendances lues : le plateau suit la géométrie du panneau).
    readonly property point origin: {
        if (!panel || !tray.parent)
            return Qt.point(0, 0)
        const w = panel.width, h = panel.height, px = panel.x, py = panel.y, v = panel.visible
        return panel.mapToItem(tray.parent, 0, 0)
    }
    x: !panel ? 0 : panel.side === "right" ? origin.x - width - 6 : origin.x + panel.width + 6
    y: origin.y + 6

    function dropAt(scene, actionId) {
        if (!model || !view)
            return false
        return model.dropAt(view.sceneToPage(Qt.point(scene.x, scene.y)), actionId, view.zoom)
    }

    Component.onCompleted: if (view) view.addInputCover(tray)
    onViewChanged: if (view) view.addInputCover(tray)

    Text {
        x: tray.margin
        y: 2
        height: tray.headerHeight
        verticalAlignment: Text.AlignVCenter
        text: qsTr("Glisser-déposer")
        textFormat: Text.PlainText
        color: Theme.texte
        font.pixelSize: Theme.taillePolice
        font.bold: true
    }

    Grid {
        id: grid
        objectName: "dragDropGrid"
        x: tray.margin
        y: tray.headerHeight + tray.margin
        columns: tray.columns
        readonly property int count: tray.model ? tray.model.rowCount() : 0

        Repeater {
            model: tray.model
            delegate: Item {
                id: slot
                required property string id
                required property string label
                required property string icon
                width: tray.cell
                height: tray.cell
                z: drag.active ? 10 : 0
                Accessible.name: slot.label

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 2
                    radius: Theme.rayon
                    color: hover.hovered ? Theme.surfaceSurvol : Theme.surface
                    Image {
                        anchors.centerIn: parent
                        source: "qrc:/qt/qml/ActCraft/Ui/icons/" + slot.icon + ".svg"
                        sourceSize.width: Theme.tailleIcone
                        sourceSize.height: Theme.tailleIcone
                        width: Theme.tailleIcone
                        height: Theme.tailleIcone
                    }
                }
                HoverHandler { id: hover }

                // Copie qui suit le pointeur ; l'icône du plateau reste en place.
                Rectangle {
                    id: ghost
                    width: tray.cell - 8
                    height: tray.cell - 8
                    x: 4
                    y: 4
                    visible: drag.active
                    radius: Theme.rayon
                    color: Theme.palette
                    border.color: Theme.surfaceActive
                    border.width: 2
                    opacity: 0.9
                    Image {
                        anchors.centerIn: parent
                        source: "qrc:/qt/qml/ActCraft/Ui/icons/" + slot.icon + ".svg"
                        sourceSize.width: Theme.tailleIcone
                        sourceSize.height: Theme.tailleIcone
                        width: Theme.tailleIcone
                        height: Theme.tailleIcone
                    }
                }
                DragHandler {
                    id: drag
                    target: ghost
                    onActiveChanged: {
                        if (active)
                            return
                        const scene = ghost.mapToItem(null, ghost.width / 2, ghost.height / 2)
                        ghost.x = 4
                        ghost.y = 4
                        tray.dropAt(scene, slot.id)
                    }
                }
            }
        }
    }
}
