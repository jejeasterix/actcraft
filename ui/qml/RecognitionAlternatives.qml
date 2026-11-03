import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Reconnaissance (F-295 et F-296, L7-T21 et L7-T23) : porte le contrôleur et les gestionnaires de
// `tool.shape-recognition` et `tool.handwriting` (cochables, exclusifs), et de `recognition.convert-to-shape`.
// La commande d'écriture est grisée tant que le moteur n'a pas répondu disponible (le contrôleur possède le fil
// de reconnaissance et l'interroge au démarrage) ; absente d'Android, elle n'interroge rien. Les autres lectures
// du dernier texte reconnu s'affichent dans un FloatingPanel près du texte : couverture de la page (ni stylet,
// ni doigt, ni souris ne tracent dessous) ; les choisir remplace le texte, la croix valide.
Item {
    id: root
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property Item pageArea: null
    property QtObject controller: recognitionController
    property bool probed: false

    RecognitionController {
        id: recognitionController
        objectName: "recognition"
        documentController: root.documentController
        tools: root.tools
    }

    FloatingPanel {
        id: alternativesPanel
        objectName: "recognitionAlternatives"
        // Couche plein cadre de la fenêtre (comme les autres panneaux) : le panneau se place en coordonnées de la couche.
        parent: root.parent
        z: root.z
        pageArea: root.pageArea
        title: qsTr("Autres lectures")
        visible: false
        width: 220
        height: Theme.tailleBouton * (recognitionController.alternatives.length + 1) + 2 * Theme.espace2
        onClosed: recognitionController.validate()

        Column {
            anchors.fill: parent
            Repeater {
                model: recognitionController.alternatives
                delegate: Button {
                    required property string modelData
                    required property int index
                    objectName: "recognitionAlternative"
                    width: parent ? parent.width : 0
                    height: Theme.tailleBouton
                    flat: true
                    text: modelData
                    Accessible.name: modelData
                    onClicked: recognitionController.chooseAlternative(index)
                }
            }
        }
    }

    // Sous le dernier texte reconnu, borné à la couche.
    function placeAlternatives() {
        const view = root.pageArea ? root.pageArea.view : null
        const layer = alternativesPanel.parent
        if (!view || !layer) {
            return
        }
        const anchor = recognitionController.alternativesAnchor
        const corner = layer.mapFromItem(null, view.pageToScene(Qt.point(anchor.x, anchor.y + anchor.height)))
        alternativesPanel.x = Math.max(0, Math.min(corner.x, layer.width - alternativesPanel.width))
        alternativesPanel.y = Math.max(0, Math.min(corner.y + 8, layer.height - alternativesPanel.height))
    }

    function registerCommands() {
        if (!root.commands) {
            return
        }
        root.commands.setHandler("tool.shape-recognition", function () {
            recognitionController.shapeMode = !recognitionController.shapeMode // la coche suit shapeModeChanged
        })
        root.commands.setHandler("tool.handwriting", function () {
            recognitionController.handwritingMode = !recognitionController.handwritingMode
        })
        root.commands.setHandler("recognition.convert-to-shape", function () {
            recognitionController.convertToShape() // la sélection, sinon les traits en attente
        })
        root.commands.setHandler("recognition.convert-to-text", function () {
            recognitionController.convertToText()
        })
        syncChecked()
        root.commands.setEnabled("tool.handwriting", recognitionController.handwritingAvailable)
        if (!root.probed && root.commands.isAvailable("tool.handwriting")) {
            root.probed = true
            recognitionController.probeHandwriting()
        }
    }

    function syncChecked() {
        if (root.commands) {
            root.commands.setChecked("tool.shape-recognition", recognitionController.shapeMode)
            root.commands.setChecked("tool.handwriting", recognitionController.handwritingMode)
        }
    }

    Connections {
        target: recognitionController
        function onShapeModeChanged() { root.syncChecked() }
        function onHandwritingModeChanged() { root.syncChecked() }
        function onHandwritingProbed() {
            if (root.commands) {
                // Moteur indisponible : grisée ; l'explication est recognitionController.handwritingStatus.
                root.commands.setEnabled("tool.handwriting", recognitionController.handwritingAvailable)
            }
        }
        function onAlternativesChanged() {
            alternativesPanel.visible = recognitionController.alternatives.length > 0
            root.placeAlternatives()
        }
    }
    onCommandsChanged: registerCommands()

    Component.onCompleted: registerCommands()
}
