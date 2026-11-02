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
    property int chosen: 0

    function choose(rank) {
        root.chosen = rank
        root.commands.trigger("connector.style", rank)
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
        }
    }
}
