import QtQuick
import ActCraft.Ui

// Outils de page (F-591 à F-593, L7-T6) : les réglages de présentation de la page affichée (outil, rideau, spot)
// s'appliquent au rideau et au spot à chaque changement de page et quand le document est remplacé, jamais après
// une simple modification (le rideau tiré à la main reste où il est). Ouvrir le spot ferme le rideau. Les
// commandes page.* écrivent les réglages de la page affichée, une étape chacune. Item non visuel.
Item {
    id: root
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property Item pageArea: null
    property QtObject controller: pageToolsController
    property QtObject curtain: null
    property QtObject spotlight: null

    PageToolsController {
        id: pageToolsController
        objectName: "pageTools"
        documentController: root.documentController
        curtain: root.curtain
        spotlight: root.spotlight
    }

    function registerCommands() {
        if (!root.commands) {
            return
        }
        root.commands.setHandler("page.tools-mode", function (mode) {
            pageToolsController.setToolsMode(String(mode))
        })
        root.commands.setHandler("page.curtain-mode", function (mode) {
            pageToolsController.setCurtainMode(String(mode))
        })
        root.commands.setHandler("page.curtain-distance", function (distance) {
            pageToolsController.setCurtainDistance(Number(distance))
        })
        root.commands.setHandler("page.spotlight-mode", function (mode) {
            pageToolsController.setSpotShape(String(mode))
        })
    }

    onCommandsChanged: registerCommands()

    Component.onCompleted: registerCommands()
}
