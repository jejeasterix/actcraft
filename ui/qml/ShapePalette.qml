import QtQuick
import QtQuick.Layouts
import ActCraft.Ui

// Palette des formes (F-240, S7.1) : panneau vertical à droite, visible quand l'outil courant est « shape ».
// Nuancier du remplissage 2 × 4 (P38, L4-C7) : 1re case, le remplissage par défaut de l'outil ; six couleurs de
// la disposition ; 8e case « sans remplissage », une valeur du remplissage (la couleur choisie est gardée). Le
// contour se règle par la palette principale, outil des formes actif (ToolHost::setColor). Bascule « depuis le
// centre », grille des formes dessinées d'après notre propre géométrie (appTools.shapeOutline, contour normalisé
// dans [0, 1]) ; la forme courante est encadrée. Boutons : « shape-<préréglage> ».
Rectangle {
    id: root
    objectName: "shapePalette"
    required property Tools tools
    required property Commands commands
    readonly property int cell: 32
    // Sept couleurs puis la case « sans remplissage » (chaîne vide).
    readonly property var fillSwatches: [String(root.tools.shapeDefaultFillColor)].concat(
        root.commands.colors.slice(0, 6), [""])

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
            text: qsTr("Remplissage")
            color: Theme.texteDiscret
            font.pixelSize: Theme.taillePoliceTitre
        }

        Grid {
            objectName: "shapeSwatches"
            columns: 4
            spacing: Theme.espace1
            Repeater {
                model: root.fillSwatches
                delegate: Rectangle {
                    id: swatch
                    required property string modelData
                    required property int index
                    readonly property bool unfilled: modelData === ""
                    readonly property bool current: unfilled
                        ? !root.tools.shapeFilled
                        : root.tools.shapeFilled && Qt.colorEqual(root.tools.shapeFillColor, modelData)
                    objectName: "shape-swatch-" + index
                    width: root.cell
                    height: Theme.taillePastille + Theme.espace2
                    radius: 3
                    color: unfilled ? Theme.surface : modelData
                    border.width: current ? 2 : 1
                    border.color: current ? Theme.cadreSelection : Theme.contour
                    function choose() {
                        if (unfilled) {
                            root.tools.setShapeFilled(false)
                        } else {
                            root.tools.setShapeFillColor(modelData)
                            root.tools.setShapeFilled(true)
                        }
                    }
                    Accessible.role: Accessible.RadioButton
                    Accessible.name: unfilled ? qsTr("Sans remplissage") : qsTr("Remplissage %1").arg(modelData)
                    Accessible.checked: current
                    Accessible.onPressAction: swatch.choose()
                    // « Sans remplissage » : une barre oblique sur le fond neutre.
                    Rectangle {
                        visible: swatch.unfilled
                        anchors.centerIn: parent
                        width: parent.width - 8
                        height: 2
                        rotation: -30
                        color: Theme.texte
                    }
                    TapHandler { onTapped: swatch.choose() }
                }
            }
        }

        Rectangle {
            id: fromCenter
            objectName: "shape-from-center"
            readonly property bool on: root.tools.shapeFromCenter
            width: parent.width
            height: Theme.tailleBouton - Theme.espace2
            radius: Theme.rayon
            color: on ? Theme.surfaceActive : Theme.surface
            Accessible.role: Accessible.CheckBox
            Accessible.name: qsTr("Depuis le centre")
            Accessible.checked: on
            Accessible.onPressAction: root.tools.setShapeFromCenter(!fromCenter.on)
            Text {
                anchors.centerIn: parent
                text: qsTr("Depuis le centre")
                color: fromCenter.on ? Theme.texteSurActive : Theme.texte
                font.pixelSize: Theme.taillePolice
            }
            TapHandler { onTapped: root.tools.setShapeFromCenter(!fromCenter.on) }
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
                    Canvas {
                        id: thumbnail
                        anchors.centerIn: parent
                        width: 22
                        height: 22
                        readonly property color ink: button.current ? Theme.texteSurActive : Theme.texte
                        onInkChanged: requestPaint()
                        onPaint: {
                            const ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            ctx.strokeStyle = ink
                            ctx.lineWidth = 1.5
                            ctx.beginPath()
                            button.outline.forEach((p, i) => {
                                if (i === 0)
                                    ctx.moveTo(1 + p.x * 20, 1 + p.y * 20)
                                else
                                    ctx.lineTo(1 + p.x * 20, 1 + p.y * 20)
                            })
                            ctx.stroke()
                        }
                    }
                    TapHandler { onTapped: root.tools.chooseShape(button.modelData) }
                }
            }
        }
    }
}
