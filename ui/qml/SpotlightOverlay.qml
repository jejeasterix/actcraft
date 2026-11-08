import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes
import ActCraft.Ui

// Spot (F-270, L7-T4) : un masque opaque qui couvre la page sauf une zone (spot rond ou carré) ou, pour un cache
// plein, la seule zone. En coordonnées de page (pageOverlays : suit le zoom et le défilement). Le masque n'est
// jamais redessiné dans le cache et n'est pas un objet de page. Il lit son état par son contrôleur et par lui seul.
// Entrée : le store est une couverture de la vue (addInputCover) tant que le spot est actif ; sa partie saisissable
// est son containmentMask (la partie masquée d'un spot, l'intérieur d'un cache plein, le pourtour dans les deux
// cas), le reste laisse passer le stylet vers la page. Le curseur suit la zone survolée. Le menu propre du spot
// (clic droit, ou appui long au doigt et au stylet) offre la capture de la zone éclairée.
Item {
    id: root
    anchors.fill: parent
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property Item pageArea: null
    property QtObject controller: spotlightController
    // Largeur du contour du cadre : celle du store (Spotlight::kDefaultOutline), jamais un nombre en dur ici.
    readonly property real outline: spotlightController.outline
    property bool capturing: false
    property bool covering: false

    SpotlightController {
        id: spotlightController
        objectName: "spotlight"
        area: Qt.size(root.width, root.height)
    }

    function rectPath(r) {
        return "M" + r.x + "," + r.y + " h" + r.width + " v" + r.height + " h" + (-r.width) + " Z "
    }

    function ellipsePath(r) {
        const rx = r.width / 2
        const ry = r.height / 2
        const cy = r.y + ry
        return "M" + r.x + "," + cy + " A" + rx + "," + ry + " 0 1 0 " + (r.x + r.width) + "," + cy
            + " A" + rx + "," + ry + " 0 1 0 " + r.x + "," + cy + " Z "
    }

    // Contour de la zone : ellipse (formes rondes : 1 et 3) ou rectangle (2 et 4).
    readonly property string holePath: {
        const r = spotlightController.frame
        const shape = spotlightController.shape
        return (shape === 1 || shape === 3) ? ellipsePath(r) : rectPath(r)
    }
    // Spot : la page entière moins la zone (remplissage pair-impair) ; cache plein : la zone seule.
    readonly property string maskPath: spotlightController.hidesInside
        ? holePath : rectPath(Qt.rect(0, 0, root.width, root.height)) + holePath

    Item {
        id: store
        objectName: "spotlightStore"
        anchors.fill: parent
        visible: spotlightController.active
        containmentMask: reach

        QtObject {
            id: reach
            function contains(point: point): bool {
                return spotlightController.covers(point)
            }
        }

        Shape {
            anchors.fill: parent
            ShapePath {
                fillRule: ShapePath.OddEvenFill
                fillColor: spotlightController.color
                strokeColor: "transparent"
                PathSvg { path: root.maskPath }
            }
            ShapePath {
                fillColor: "transparent"
                strokeColor: Theme.cadreSelection
                strokeWidth: root.outline
                PathSvg { path: root.holePath }
            }
        }

        // Tous périphériques (stylet, doigt, souris). Le store ne saisit que dans sa zone saisissable
        // (Spotlight.press) ; ailleurs le geste traverse vers la page.
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
                    holding = spotlightController.press(root.mapFromItem(null, point.scenePressPosition))
                }
                if (holding) {
                    spotlightController.move(root.mapFromItem(null, point.scenePosition))
                }
            }
            onActiveChanged: {
                if (active) {
                    return
                }
                if (holding) {
                    spotlightController.release()
                }
                started = false
                holding = false
            }
        }

        TapHandler {
            acceptedButtons: Qt.RightButton
            onTapped: eventPoint => spotMenu.popup(store, eventPoint.position.x, eventPoint.position.y)
        }
        TapHandler {
            acceptedDevices: PointerDevice.TouchScreen | PointerDevice.Stylus
            longPressThreshold: 0.7
            onLongPressed: spotMenu.popup(store, point.position.x, point.position.y)
        }
    }

    // Curseur selon la zone : déplacer sur la partie qui déplace, redimensionner sur le bord, flèche ailleurs.
    // Posée dans l'emplacement de présentation, au-dessus des couches de page : le survol que la couche des outils de
    // maths prend sur toute la page n'atteint pas une couche placée dessous. Sans masque de saisie : le survol se lit
    // partout, seul l'appui est limité à la zone saisissable. Elle ne prend aucun appui.
    Item {
        id: hoverZone
        objectName: "spotlightHoverZone"
        parent: root.pageArea ? root.pageArea.presentationSlot : null
        anchors.fill: parent
        visible: spotlightController.active
        HoverHandler {
            id: hover
            objectName: "spotlightHover"
            cursorShape: hover.hovered ? spotlightController.cursorAt(root.mapFromItem(hoverZone, hover.point.position))
                                       : Qt.ArrowCursor
        }
    }

    Menu {
        id: spotMenu
        objectName: "spotlightMenu"
        MenuItem {
            objectName: "spotlightCaptureEntry"
            text: qsTr("Capturer la zone éclairée")
            enabled: spotlightController.active && !spotlightController.hidesInside && !root.capturing
            onTriggered: root.captureLit()
        }
    }

    // Capture de la zone éclairée : la page entière est photographiée (QQuickItem::grabToImage, au pixel de page),
    // le masque caché le temps de la prise ; le contrôleur découpe la zone et l'insère en une étape. Vrai si la prise
    // est partie ; l'insertion suit à l'image suivante.
    function captureLit() {
        const view = root.pageArea ? root.pageArea.view : null
        const page = view ? view.content : null
        if (!page || root.capturing || !spotlightController.active || spotlightController.hidesInside) {
            return false
        }
        root.capturing = true
        root.visible = false
        const started = page.grabToImage(function (result) {
            root.visible = true
            root.capturing = false
            if (root.documentController) {
                spotlightController.insertCapture(result.image, root.documentController)
            }
        }, Qt.size(page.width, page.height))
        if (!started) {
            root.visible = true
            root.capturing = false
        }
        return started
    }

    function syncCover() {
        const view = root.pageArea ? root.pageArea.view : null
        if (!view) {
            return
        }
        if (spotlightController.active === root.covering) {
            return
        }
        root.covering = spotlightController.active
        if (root.covering) {
            view.addInputCover(store)
        } else {
            view.removeInputCover(store)
        }
    }

    readonly property var spotlightCommands: ({
        "present.spotlight-circle": 1,
        "present.spotlight-square": 2,
        "present.spotlight-solid-circle": 3,
        "present.spotlight-solid-square": 4
    })

    // Forme dont les coches sont posées (-1 : aucune encore). Les coches suivent la forme d'où que vienne son
    // changement (commande, changement de page par PageToolsController) : relues sur `changed`, et seulement
    // quand la forme change, pas à chaque déplacement du spot.
    property int checkedShape: -1

    function syncChecks() {
        if (!root.commands) {
            return
        }
        root.checkedShape = spotlightController.shape
        for (const id of Object.keys(root.spotlightCommands)) {
            root.commands.setChecked(id, spotlightController.shape === root.spotlightCommands[id])
        }
    }

    function bindCommand(id, shape) {
        root.commands.setHandler(id, function () {
            spotlightController.toggle(shape)
        })
    }

    function registerCommands() {
        if (!root.commands) {
            return
        }
        for (const id of Object.keys(root.spotlightCommands)) {
            root.bindCommand(id, root.spotlightCommands[id])
        }
        root.syncChecks()
    }

    Connections {
        target: spotlightController
        function onChanged() {
            root.syncCover()
            if (spotlightController.shape !== root.checkedShape) {
                root.syncChecks()
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
