import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes
import ActCraft.Ui

// Palette des formes (F-240, S7.1) : panneau vertical à droite, visible quand l'outil courant est « shape ».
// Nuancier du contour (les couleurs de la disposition), case « sans remplissage », bascule « depuis le centre »,
// grille des formes dessinées d'après notre propre géométrie (appTools.shapeOutline, contour normalisé dans
// [0, 1]) ; la forme courante est encadrée. Boutons : « shape-<préréglage> ».
Rectangle {
    id: root
    objectName: "shapePalette"
    required property Tools tools
    required property Commands commands
    readonly property int swatchCount: 8
    readonly property int cell: 32

    visible: root.tools !== null && root.tools.currentTool === "shape"
    Layout.fillHeight: true
    Layout.preferredWidth: 4 * root.cell + 3 * Theme.espace1 + 2 * Theme.espace2
    color: Theme.palette

    Column {
        x: Theme.espace2
        y: Theme.espace2
        width: parent.width - 2 * Theme.espace2
        spacing: Theme.espace2

        Text {
            text: qsTr("Contour")
            color: Theme.texteDiscret
            font.pixelSize: Theme.taillePoliceTitre
        }

        // Nuancier du contour : 2 × 4, tiré des couleurs de la disposition.
        Grid {
            objectName: "shapeSwatches"
            columns: 4
            spacing: Theme.espace1
            Repeater {
                model: root.commands.colors.slice(0, root.swatchCount)
                delegate: Rectangle {
                    id: swatch
                    required property string modelData
                    required property int index
                    readonly property bool current: Qt.colorEqual(root.tools.color, modelData)
                    objectName: "shape-swatch-" + index
                    width: root.cell
                    height: Theme.taillePastille + Theme.espace2
                    radius: 3
                    color: modelData
                    border.width: current ? 2 : 1
                    border.color: current ? Theme.cadreSelection : Theme.contour
                    Accessible.role: Accessible.RadioButton
                    Accessible.name: qsTr("Contour %1").arg(modelData)
                    Accessible.checked: current
                    Accessible.onPressAction: root.tools.setColor(swatch.modelData)
                    TapHandler { onTapped: root.tools.setColor(swatch.modelData) }
                }
            }
        }

        // Case « sans remplissage » et bascule « depuis le centre » : deux interrupteurs d'une ligne.
        Repeater {
            model: [
                { id: "shape-unfilled", label: qsTr("Sans remplissage"), on: !root.tools.shapeFilled },
                { id: "shape-from-center", label: qsTr("Depuis le centre"), on: root.tools.shapeFromCenter }
            ]
            delegate: Rectangle {
                id: toggle
                required property var modelData
                objectName: modelData.id
                width: parent.width
                height: Theme.tailleBouton - Theme.espace2
                radius: Theme.rayon
                color: modelData.on ? Theme.surfaceActive : Theme.surface
                Accessible.role: Accessible.CheckBox
                Accessible.name: modelData.label
                Accessible.checked: modelData.on
                function flip() {
                    if (modelData.id === "shape-unfilled")
                        root.tools.setShapeFilled(modelData.on)
                    else
                        root.tools.setShapeFromCenter(!modelData.on)
                }
                Accessible.onPressAction: flip()
                Text {
                    anchors.centerIn: parent
                    text: toggle.modelData.label
                    color: toggle.modelData.on ? Theme.texteSurActive : Theme.texte
                    font.pixelSize: Theme.taillePolice
                }
                TapHandler { onTapped: toggle.flip() }
            }
        }

        Grid {
            objectName: "shapeGrid"
            columns: 4
            spacing: Theme.espace1
            Repeater {
                model: root.tools.shapePresets
                delegate: Rectangle {
                    id: button
                    required property string modelData
                    readonly property bool current: root.tools.shapePreset === modelData
                    readonly property var outline: root.tools.shapeOutline(modelData)
                    objectName: "shape-" + modelData
                    width: root.cell
                    height: root.cell
                    radius: 3
                    color: current ? Theme.surfaceActive : Theme.surface
                    border.width: current ? 2 : 0
                    border.color: Theme.cadreSelection
                    Accessible.role: Accessible.RadioButton
                    Accessible.name: modelData
                    Accessible.checked: current
                    Accessible.onPressAction: root.tools.chooseShape(button.modelData)
                    Shape {
                        anchors.centerIn: parent
                        width: 22
                        height: 22
                        ShapePath {
                            strokeColor: button.current ? Theme.texteSurActive : Theme.texte
                            strokeWidth: 1.5
                            fillColor: "transparent"
                            PathPolyline {
                                path: button.outline.map(p => Qt.point(1 + p.x * 20, 1 + p.y * 20))
                            }
                        }
                    }
                    TapHandler { onTapped: root.tools.chooseShape(button.modelData) }
                }
            }
        }
    }
}
