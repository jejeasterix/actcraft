import QtQuick
import ActCraft.Ui

// Reconnaissance (F-296, L7-T21) : porte le contrôleur et les gestionnaires de `tool.shape-recognition`
// (cochable) et de `recognition.convert-to-shape`. Ne prend aucune entrée : les alternatives d'écriture, qui
// en prendront, viennent avec L7-T23.
Item {
    id: root
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property Item pageArea: null
    property QtObject controller: recognitionController

    RecognitionController {
        id: recognitionController
        objectName: "recognition"
        documentController: root.documentController
    }

    function registerCommands() {
        if (!root.commands) {
            return
        }
        root.commands.setHandler("tool.shape-recognition", function () {
            recognitionController.shapeMode = !recognitionController.shapeMode // la coche suit shapeModeChanged
        })
        root.commands.setHandler("recognition.convert-to-shape", function () {
            recognitionController.convertPending()
        })
        root.commands.setChecked("tool.shape-recognition", recognitionController.shapeMode)
    }

    Connections {
        target: recognitionController
        function onShapeModeChanged() {
            if (root.commands) {
                root.commands.setChecked("tool.shape-recognition", recognitionController.shapeMode)
            }
        }
    }
    onCommandsChanged: registerCommands()

    Component.onCompleted: registerCommands()
}
