import QtQuick
import QtQuick.Layouts
import ActCraft.Ui

// Panneau du connecteur (F-245), ancré à droite, visible avec l'outil connecteur : quatorze styles numérotés
// « Style 1 » à « Style 14 » (décision H5-18 : ni nom ni dessin tant que leur définition manque). Le style 1,
// la ligne simple, est le rang 0 et le défaut. Un clic appelle connector.style avec le rang 0..13.
Rectangle {
    id: root
    objectName: "connectorPanel"
    required property Commands commands
    required property Tools tools

    readonly property int styleCount: 14
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
                    objectName: "connectorStyle-" + index
                    width: parent.width
                    height: Theme.tailleBouton
                    radius: Theme.rayon
                    color: current ? Theme.surfaceActive : Theme.surface
                    border.width: 1
                    border.color: Theme.contour
                    Accessible.role: Accessible.RadioButton
                    Accessible.name: qsTr("Style %1").arg(index + 1)
                    Accessible.checked: current
                    Text {
                        anchors.centerIn: parent
                        text: qsTr("Style %1").arg(entry.index + 1)
                        font.pixelSize: Theme.taillePolice
                        color: entry.current ? Theme.texteSurActive : Theme.texte
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
                    readonly property bool endA: index === 0
                    width: parent.width
                    spacing: Theme.espace1
                    Text {
                        text: capGroup.endA ? qsTr("Extrémité A") : qsTr("Extrémité B")
                        font.pixelSize: Theme.taillePolice
                        color: Theme.texte
                    }
                    Repeater {
                        model: root.capNames
                        delegate: Rectangle {
                            id: capEntry
                            required property int index
                            required property string modelData
                            readonly property bool current: (capGroup.endA ? root.capA : root.capB) === index
                            objectName: (capGroup.endA ? "connectorCapA-" : "connectorCapB-") + index
                            width: parent.width
                            height: Theme.tailleBouton
                            radius: Theme.rayon
                            color: current ? Theme.surfaceActive : Theme.surface
                            border.width: 1
                            border.color: Theme.contour
                            Accessible.role: Accessible.RadioButton
                            Accessible.name: (capGroup.endA ? qsTr("Extrémité A : %1") : qsTr("Extrémité B : %1")).arg(modelData)
                            Accessible.checked: current
                            Text {
                                anchors.centerIn: parent
                                text: capEntry.modelData
                                font.pixelSize: Theme.taillePolice
                                color: capEntry.current ? Theme.texteSurActive : Theme.texte
                            }
                            TapHandler {
                                onTapped: capGroup.endA ? root.chooseCaps(capEntry.index, root.capB)
                                                        : root.chooseCaps(root.capA, capEntry.index)
                            }
                        }
                    }
                }
            }
        }
    }
}
