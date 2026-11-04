import QtQuick
import QtQuick.Layouts
import ActCraft.Ui

// Panneau du connecteur (F-245), ancré à droite, visible avec l'outil connecteur : quatorze styles, chacun
// nommé d'après son dessin et montré par une vignette (L5-C3), dans l'ordre des rangs de core
// (connectorStyle). La ligne simple est le rang 0 et le défaut. Un clic appelle connector.style avec le rang
// 0..13, qui règle aussi les extrémités selon les flèches du style (H5-33).
Rectangle {
    id: root
    objectName: "connectorPanel"
    required property Commands commands
    required property Tools tools

    readonly property int styleCount: 14
    readonly property var styleNames: [
        qsTr("Ligne"),
        qsTr("Flèche au départ"),
        qsTr("Flèche à l'arrivée"),
        qsTr("Deux flèches"),
        qsTr("Pointillés"),
        qsTr("Pointillés, flèche au départ"),
        qsTr("Pointillés, flèche à l'arrivée"),
        qsTr("Pointillés, deux flèches"),
        qsTr("Courbe, départ vertical"),
        qsTr("Courbe, départ horizontal"),
        qsTr("Marches"),
        qsTr("Coude"),
        qsTr("Oblique puis horizontal"),
        qsTr("Oblique puis horizontal, flèche au départ")
    ]
    // M18 (L5-C18) : le panneau montre la mémoire de l'outil (relue au démarrage), pas un rang à lui.
    readonly property int chosen: root.tools.connectorStyle
    readonly property int capA: root.tools.connectorCapA
    readonly property int capB: root.tools.connectorCapB
    readonly property var capNames: [qsTr("Aucune"), qsTr("Rayon"), qsTr("Flèche")]

    function choose(rank) {
        root.commands.trigger("connector.style", rank)
    }
    // Extrémités A et B (D11, M3 de L5-C18) : rangs 0 aucune, 1 rayon, 2 flèche ; connector.caps reçoit « A,B ».
    function chooseCaps(a, b) {
        root.commands.trigger("connector.caps", a + "," + b)
    }

    visible: root.tools.currentTool === "connector"
    Layout.fillHeight: true
    Layout.preferredWidth: 96 + 2 * Theme.espace2
    color: Theme.palette

    Flickable {
        anchors.fill: parent
        contentHeight: column.height + 2 * Theme.espace2
        clip: true
        Column {
            id: column
            x: Theme.espace2
            y: Theme.espace2
            width: parent.width - 2 * Theme.espace2
            spacing: Theme.espace1
            Repeater {
                model: root.styleCount
                delegate: Rectangle {
                    id: entry
                    required property int index
                    readonly property bool current: root.chosen === index
                    readonly property string label: root.styleNames[index]
                    readonly property color ink: current ? Theme.texteSurActive : Theme.texte
                    objectName: "connectorStyle-" + index
                    width: parent.width
                    height: Math.max(Theme.tailleBouton, entryColumn.implicitHeight + 2 * Theme.espace1)
                    radius: Theme.rayon
                    color: current ? Theme.surfaceActive : Theme.surface
                    border.width: 1
                    border.color: Theme.contour
                    Accessible.role: Accessible.RadioButton
                    Accessible.name: entry.label
                    Accessible.checked: current
                    Column {
                        id: entryColumn
                        anchors.centerIn: parent
                        width: parent.width - 2 * Theme.espace1
                        spacing: 2
                        // Vignette : le tracé de core (Tools.connectorStyleSketch), flèches et pointillés du style.
                        Canvas {
                            id: sketch
                            readonly property var drawing: root.tools.connectorStyleSketch(entry.index, width, height)
                            objectName: "connectorSketch-" + entry.index
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: 44
                            height: 24
                            onDrawingChanged: requestPaint()
                            Connections {
                                target: entry
                                function onInkChanged() { sketch.requestPaint() }
                            }
                            onPaint: {
                                const ctx = getContext("2d")
                                ctx.reset()
                                const points = sketch.drawing.points
                                if (!points || points.length < 2)
                                    return
                                ctx.strokeStyle = entry.ink
                                ctx.fillStyle = entry.ink
                                ctx.lineWidth = 1.5
                                if (sketch.drawing.dashed && ctx.setLineDash)
                                    ctx.setLineDash([3, 2])
                                ctx.beginPath()
                                ctx.moveTo(points[0].x, points[0].y)
                                for (let i = 1; i < points.length; ++i)
                                    ctx.lineTo(points[i].x, points[i].y)
                                ctx.stroke()
                                const head = function (tip, outward) {
                                    const size = 6
                                    const baseX = tip.x - outward.x * size
                                    const baseY = tip.y - outward.y * size
                                    ctx.beginPath()
                                    ctx.moveTo(tip.x, tip.y)
                                    ctx.lineTo(baseX - outward.y * size / 2, baseY + outward.x * size / 2)
                                    ctx.lineTo(baseX + outward.y * size / 2, baseY - outward.x * size / 2)
                                    ctx.closePath()
                                    ctx.fill()
                                }
                                if (sketch.drawing.arrowStart)
                                    head(points[0], sketch.drawing.outwardA)
                                if (sketch.drawing.arrowEnd)
                                    head(points[points.length - 1], sketch.drawing.outwardB)
                            }
                        }
                        Text {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            text: entry.label
                            font.pixelSize: Theme.taillePolice
                            color: entry.ink
                        }
                    }
                    TapHandler {
                        onTapped: root.choose(entry.index)
                    }
                }
            }
            Repeater {
                model: 2 // extrémité A, puis B
                delegate: Column {
                    id: capGroup
                    required property int index
                    readonly property bool startSide: index === 0
                    width: parent.width
                    spacing: Theme.espace1
                    Text {
                        text: capGroup.startSide ? qsTr("Extrémité A") : qsTr("Extrémité B")
                        font.pixelSize: Theme.taillePolice
                        color: Theme.texte
                    }
                    Repeater {
                        model: root.capNames
                        delegate: Rectangle {
                            id: capEntry
                            required property int index
                            required property string modelData
                            readonly property bool current: (capGroup.startSide ? root.capA : root.capB) === index
                            objectName: (capGroup.startSide ? "connectorCapA-" : "connectorCapB-") + index
                            width: parent.width
                            height: Theme.tailleBouton
                            radius: Theme.rayon
                            color: current ? Theme.surfaceActive : Theme.surface
                            border.width: 1
                            border.color: Theme.contour
                            Accessible.role: Accessible.RadioButton
                            Accessible.name: (capGroup.startSide ? qsTr("Extrémité A : %1") : qsTr("Extrémité B : %1")).arg(modelData)
                            Accessible.checked: current
                            Text {
                                anchors.centerIn: parent
                                text: capEntry.modelData
                                font.pixelSize: Theme.taillePolice
                                color: capEntry.current ? Theme.texteSurActive : Theme.texte
                            }
                            TapHandler {
                                onTapped: capGroup.startSide ? root.chooseCaps(capEntry.index, root.capB)
                                                        : root.chooseCaps(root.capA, capEntry.index)
                            }
                        }
                    }
                }
            }
        }
    }
}
