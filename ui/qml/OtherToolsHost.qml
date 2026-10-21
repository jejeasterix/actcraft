import QtQuick
import ActCraft.Ui

// Autres outils (L7-T17) : date et heure sur la page (insert.date-time, F-300) et navigateur web
// (tool.web-browser, F-299). Item non visuel ; les gestionnaires s'enregistrent quand les commandes arrivent.
Item {
    id: root
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property Item pageArea: null
    property QtObject controller: presentationCommands

    PresentationCommands {
        id: presentationCommands
        objectName: "presentationCommands"
        documentController: root.documentController
    }

    function registerCommands() {
        if (!root.commands) {
            return
        }
        root.commands.setHandler("insert.date-time", function () {
            if (root.controller) {
                root.controller.insertDateTime()
            }
        })
        root.commands.setHandler("tool.web-browser", function () {
            if (root.controller) {
                root.controller.openWebHome()
            }
        })
    }

    onCommandsChanged: registerCommands()
    Component.onCompleted: registerCommands()
}
