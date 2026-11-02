import QtQuick
import QtQuick.Layouts
import ActCraft.Ui

// Barre des modifications du stylo (F-221, D12), verticale, à côté de la palette (visible avec
// view.pen-modifiers) : un bouton par tracé ; un nouveau clic sur le tracé courant revient au trait libre. Tout
// passe par la commande pen.shape (argument : l'identifiant du tracé, « free » pour le trait libre).
Rectangle {
    id: root
    required property Commands commands
    required property Tools tools

    readonly property var shapes: [
        { id: "horizontal", label: qsTr("Horizontale") },
        { id: "vertical", label: qsTr("Verticale") },
        { id: "square", label: qsTr("Carré") },
        { id: "rectangle", label: qsTr("Rectangle") },
        { id: "circle", label: qsTr("Cercle") },
        { id: "ellipse", label: qsTr("Ellipse") },
        { id: "arc", label: qsTr("Arc") },
        { id: "polyline", label: qsTr("Ligne brisée") },
        { id: "arrow", label: qsTr("Flèche") },
        { id: "zigzag", label: qsTr("Zigzag") }
    ]
    // Tracé choisi par cette barre ; vide au trait libre ou hors de l'outil des tracés.
    property string chosen: ""
    readonly property bool shapeToolActive: root.tools.currentTool === "pen-shape"
    onShapeToolActiveChanged: if (!shapeToolActive) chosen = ""

    function choose(id) {
        const next = root.chosen === id ? "free" : id
        root.commands.trigger("pen.shape", next)
        root.chosen = next === "free" ? "" : next
    }

    visible: false
    Layout.fillHeight: true
    Layout.preferredWidth: 96 + 2 * Theme.espace2
    color: Theme.palette

    Column {
        x: Theme.espace2
        y: Theme.espace2
        width: parent.width - 2 * Theme.espace2
        spacing: Theme.espace1
        Repeater {
            model: root.shapes
            delegate: Rectangle {
                id: button
                required property var modelData
                readonly property bool current: root.chosen === modelData.id
                objectName: "penShape-" + modelData.id
                width: parent.width
                height: Theme.tailleBouton
                radius: Theme.rayon
                color: current ? Theme.surfaceActive : Theme.surface
                border.width: 1
                border.color: Theme.contour
                Accessible.role: Accessible.Button
                Accessible.name: modelData.label
                Accessible.checked: current
                Text {
                    anchors.centerIn: parent
                    text: button.modelData.label
                    font.pixelSize: Theme.taillePolice
                    color: button.current ? Theme.texteSurActive : Theme.texte
                }
                TapHandler {
                    onTapped: root.choose(button.modelData.id)
                }
            }
        }
    }
}
