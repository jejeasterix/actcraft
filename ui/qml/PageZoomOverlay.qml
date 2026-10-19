import QtQuick
import ActCraft.Ui

// Zoom de page (F-260, L7-T5) : tant que le mode est actif, maintenir le stylet, le doigt ou le bouton gauche
// sur la page zoome autour du point jusqu'au relâché ; le bouton droit dézoome ; glisser fait un panoramique ;
// un double appui revient à « Ajuster ». Le document n'est jamais modifié et le cache n'est redessiné qu'une
// fois, au relâché (la vue le fait elle-même).
// Entrée : la racine couvre toute la zone et est une couverture de la vue (addInputCover) tant que le mode est
// actif : aucun trait ne part, et le bouton droit ne demande pas le menu contextuel.
Item {
    id: root
    anchors.fill: parent
    visible: zoomController.active
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property Item pageArea: null
    property QtObject controller: zoomController

    PageZoomController {
        id: zoomController
        objectName: "pageZoom"
        view: root.pageArea ? root.pageArea.view : null
    }

    HoverHandler {
        cursorShape: zoomController.panning ? Qt.ClosedHandCursor : Qt.PointingHandCursor
    }

    // Tous périphériques, boutons gauche et droit : un doigt ou un stylet compte comme le bouton gauche.
    PointHandler {
        id: grabArea
        property bool started: false
        acceptedDevices: PointerDevice.AllDevices
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPointChanged: {
            if (!active) {
                return
            }
            if (!started) {
                started = true
                const buttons = point.pressedButtons
                const rightOnly = (buttons & Qt.RightButton) !== 0 && (buttons & Qt.LeftButton) === 0
                zoomController.press(point.scenePressPosition, rightOnly)
            }
            zoomController.move(point.scenePosition)
        }
        onActiveChanged: {
            if (active) {
                return
            }
            zoomController.release()
            started = false
        }
    }

    function syncCover() {
        const view = root.pageArea ? root.pageArea.view : null
        if (!view) {
            return
        }
        if (zoomController.active) {
            view.addInputCover(root)
        } else {
            view.removeInputCover(root)
        }
    }

    function registerCommands() {
        if (!root.commands) {
            return
        }
        root.commands.setHandler("view.page-zoom", function () {
            zoomController.toggle()
            if (root.commands) {
                root.commands.setChecked("view.page-zoom", zoomController.active)
            }
        })
    }

    Connections {
        target: zoomController
        function onActiveChanged() { root.syncCover() }
    }
    onPageAreaChanged: syncCover()
    onCommandsChanged: registerCommands()

    Component.onCompleted: {
        registerCommands()
        syncCover()
    }
}
