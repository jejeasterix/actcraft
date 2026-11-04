import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Rideau (F-265, L7-T3) : un store opaque qui couvre la page, en coordonnées de page (pageOverlays : suit le
// zoom et le défilement). Glisser son bord le découvre ; le cache n'est jamais redessiné. Le rideau n'est pas un
// objet de page : il n'entre pas dans le document. Il lit son état par son contrôleur et par lui seul.
// Entrée : le store est une couverture de la vue (addInputCover) tant que le rideau est actif ; sa partie
// couvrante est son containmentMask, la partie découverte laisse passer le stylet vers la page. Le menu propre du
// rideau (clic droit, ou appui long au doigt et au stylet) offre « Mémoriser pour cette page » (L7-T6).
Item {
    id: root
    anchors.fill: parent
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property Item pageArea: null
    property QtObject controller: curtainController

    CurtainController {
        id: curtainController
        objectName: "curtain"
        area: Qt.size(root.width, root.height)
    }

    CurtainPageMemory {
        id: pageMemory
        documentController: root.documentController
        curtain: curtainController
    }

    // Poignée ronde posée au bord saisissable, à l'intérieur du store.
    component Grip: Rectangle {
        property bool across: true // vrai : bord horizontal (haut ou bas)
        width: Theme.tailleBouton - Theme.espace2
        height: width
        radius: width / 2
        color: grabArea.holding ? Theme.surfaceActive : Theme.surface
        border.width: 1
        border.color: Theme.contour
        Rectangle {
            anchors.centerIn: parent
            width: parent.across ? parent.width / 2 : 2
            height: parent.across ? 2 : parent.height / 2
            color: grabArea.holding ? Theme.texteSurActive : Theme.texte
        }
    }

    Item {
        id: store
        objectName: "curtainStore"
        anchors.fill: parent
        visible: curtainController.active
        containmentMask: veil

        Rectangle {
            id: veil
            objectName: "curtainVeil"
            x: curtainController.covered.x
            y: curtainController.covered.y
            width: curtainController.covered.width
            height: curtainController.covered.height
            color: curtainController.color

            readonly property real inset: Theme.espace1
            readonly property bool topOpen: y > 0.5
            readonly property bool bottomOpen: y + height < root.height - 0.5
            readonly property bool leftOpen: x > 0.5
            readonly property bool rightOpen: x + width < root.width - 0.5

            Grip {
                x: (veil.width - width) / 2
                y: veil.inset
                visible: veil.topOpen || !(veil.bottomOpen || veil.leftOpen || veil.rightOpen)
            }
            Grip {
                x: (veil.width - width) / 2
                y: veil.height - height - veil.inset
                visible: veil.bottomOpen
            }
            Grip {
                across: false
                x: veil.inset
                y: (veil.height - height) / 2
                visible: veil.leftOpen
            }
            Grip {
                across: false
                x: veil.width - width - veil.inset
                y: (veil.height - height) / 2
                visible: veil.rightOpen
            }
        }

        // Tous périphériques (stylet, doigt, souris). Le store ne saisit un bord que si l'appui est à sa portée
        // (Curtain.press) ; sinon le geste n'a aucun effet, et la vue ne verse rien sous le store.
        PointHandler {
            id: grabArea
            property bool started: false
            property bool holding: false
            acceptedDevices: PointerDevice.AllDevices
            onPointChanged: {
                if (!active) {
                    return
                }
                if (!started) {
                    started = true
                    holding = curtainController.press(root.mapFromItem(null, point.scenePressPosition))
                }
                if (holding) {
                    curtainController.move(root.mapFromItem(null, point.scenePosition))
                }
            }
            onActiveChanged: {
                if (active) {
                    return
                }
                if (holding) {
                    curtainController.release()
                }
                started = false
                holding = false
            }
        }

        TapHandler {
            acceptedButtons: Qt.RightButton
            onTapped: eventPoint => curtainMenu.popup(store, eventPoint.position.x, eventPoint.position.y)
        }
        TapHandler {
            acceptedDevices: PointerDevice.TouchScreen | PointerDevice.Stylus
            longPressThreshold: 0.7
            onLongPressed: curtainMenu.popup(store, point.position.x, point.position.y)
        }
    }

    Menu {
        id: curtainMenu
        objectName: "curtainMenu"
        MenuItem {
            objectName: "curtainRememberEntry"
            text: qsTr("Mémoriser pour cette page")
            enabled: curtainController.active && root.documentController !== null
            onTriggered: root.rememberForPage()
        }
    }

    // Le côté et la distance du rideau deviennent les réglages de la page affichée, en une étape.
    function rememberForPage() {
        return pageMemory.remember()
    }

    function syncCover() {
        const view = root.pageArea ? root.pageArea.view : null
        if (!view) {
            return
        }
        if (curtainController.active) {
            view.addInputCover(store)
        } else {
            view.removeInputCover(store)
        }
    }

    function registerCommands() {
        if (!root.commands) {
            return
        }
        root.commands.setHandler("present.curtain", function () {
            curtainController.toggle() // la coche suit activeChanged (Connections ci-dessous)
        })
        root.commands.setHandler("present.curtain-color", function (c) {
            curtainController.color = c
        })
    }

    Connections {
        target: curtainController
        function onActiveChanged() {
            root.syncCover()
            if (root.commands) {
                root.commands.setChecked("present.curtain", curtainController.active)
            }
        }
    }
    onPageAreaChanged: syncCover()
    onCommandsChanged: registerCommands()

    Component.onCompleted: {
        registerCommands()
        syncCover()
    }
}
