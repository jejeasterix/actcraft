import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Compas (F-282), délégué de MathToolsLayer, en coordonnées de page : pointe (le centre), bras de la pointe,
// charnière, bras de la mine, porte-mine, translucides, tournés autour de la pointe. Il ne dessine aucune encre :
// le stylo posé près de la mine verse un trait guidé par le contrôleur sur le cercle de la mine (la mine suit le
// stylo) ; centre marqué, le contrôleur ajoute ensuite une croix sur la pointe. Quand le stylo approche de la
// mine (rayon de capture du guide), la mine apparaît à la couleur de l'encre (P5). Le corps (les bras, pas leur
// boîte) est une couverture de la vue, sauf le disque de capture autour de la mine, qui va au stylo. Zones :
// déplacer par le bras de la pointe ou par la charnière, tourner sans tracer par le bras de la mine, changer le
// rayon par le porte-mine. Fermer : par le menu seulement (un geste sur le compas ne le ferme jamais).
Item {
    id: tool
    objectName: "compassTool"
    property QtObject controller: null
    property int toolIndex: -1
    property PageView pageView: null
    // Posés par MathToolsLayer : le guide de ce compas saisirait le stylo qui survole (la mine se montre), à la
    // couleur de l'encre que la vue donne au trait.
    property bool leadShown: false
    property color leadColor: "black"
    readonly property var frame: controller && controller.revision >= 0 ? controller.toolAt(toolIndex) : ({})
    readonly property real radius: frame.length || 0
    readonly property real leg: frame.thickness || 0
    readonly property bool mirrored: frame.mirrored === true
    readonly property real zoom: pageView ? Math.max(pageView.zoom, 0.01) : 1
    // Disque de capture de la mine (16 pixels d'écran ÷ zoom) ; demi-épaisseur saisissable d'un bras.
    readonly property real leadBand: controller ? controller.guideBand : 16
    readonly property real armHalf: 11
    readonly property real hingeHeight: Math.sqrt(Math.max(leg * leg - radius * radius / 4, 0))
    readonly property real reach: Math.max(radius, hingeHeight) + 60
    // Repère local : la pointe au centre de l'élément, la mine à droite (angle 0), la charnière au-dessus
    // (au-dessous si réfléchi).
    readonly property real hingeX: reach + radius / 2
    readonly property real hingeY: mirrored ? reach + hingeHeight : reach - hingeHeight
    readonly property real leadX: reach + radius
    readonly property real leadY: reach
    // Haut du porte-mine : 60 pixels de page le long du bras, depuis la mine.
    readonly property real pencilShare: leg > 0 ? Math.min(60 / leg, 1) : 0
    readonly property real holderX: leadX + (hingeX - leadX) * pencilShare
    readonly property real holderY: leadY + (hingeY - leadY) * pencilShare
    readonly property real menuX: hingeX
    readonly property real menuY: hingeY + (mirrored ? 34 : -34)
    property var grab: null

    x: (frame.centerX || 0) - reach
    y: (frame.centerY || 0) - reach
    width: 2 * reach
    height: 2 * reach
    rotation: frame.angle || 0
    transformOrigin: Item.Center

    function segmentDistance(px, py, ax, ay, bx, by) {
        const dx = bx - ax
        const dy = by - ay
        const length2 = dx * dx + dy * dy
        const t = length2 > 0 ? Math.max(0, Math.min(1, ((px - ax) * dx + (py - ay) * dy) / length2)) : 0
        return Math.hypot(px - ax - t * dx, py - ay - t * dy)
    }

    // Zone sous un point local ("" hors du corps, et dans le disque de capture de la mine) : un seul prédicat
    // pour la couverture et pour les gestes.
    function zoneAt(lx, ly) {
        if (radius <= 0)
            return ""
        if (Math.hypot(lx - leadX, ly - leadY) <= leadBand)
            return ""
        if (Math.hypot(lx - menuX, ly - menuY) <= 11)
            return "menu"
        if (Math.hypot(lx - hingeX, ly - hingeY) <= 15)
            return "move"
        if (segmentDistance(lx, ly, leadX, leadY, holderX, holderY) <= armHalf)
            return "radius"
        if (segmentDistance(lx, ly, holderX, holderY, hingeX, hingeY) <= armHalf)
            return "rotate"
        if (segmentDistance(lx, ly, reach, reach, hingeX, hingeY) <= armHalf)
            return "move"
        return ""
    }

    containmentMask: QtObject {
        function contains(point: point): bool {
            return tool.zoneAt(point.x, point.y) !== ""
        }
    }

    // Barre d'un point à un autre, en repère local.
    component Bar: Rectangle {
        property real fromX: 0
        property real fromY: 0
        property real toX: 0
        property real toY: 0
        property real thickness: 12
        x: fromX
        y: fromY - thickness / 2
        width: Math.hypot(toX - fromX, toY - fromY)
        height: thickness
        radius: thickness / 2
        transformOrigin: Item.Left
        rotation: Math.atan2(toY - fromY, toX - fromX) * 180 / Math.PI
    }

    Item {
        id: body
        objectName: "compassBody"
        anchors.fill: parent
        opacity: tool.controller ? tool.controller.defaultOpacity / 255 : 1
        Bar { fromX: tool.reach; fromY: tool.reach; toX: tool.hingeX; toY: tool.hingeY; color: "#8796a5" }
        Bar { fromX: tool.hingeX; fromY: tool.hingeY; toX: tool.holderX; toY: tool.holderY; color: "#8796a5" }
        // Porte-mine, mine comprise ; la mine à la couleur de l'encre est compassLead, hors du corps translucide.
        Bar {
            fromX: tool.holderX
            fromY: tool.holderY
            toX: tool.leadX
            toY: tool.leadY
            thickness: 10
            color: "#e0a050"
        }
        Rectangle {
            width: 30
            height: 30
            radius: 15
            x: tool.hingeX - 15
            y: tool.hingeY - 15
            color: "#9aa6b2"
            border.color: "#5b6875"
        }
        Rectangle {
            width: 6
            height: 6
            radius: 3
            x: tool.reach - 3
            y: tool.reach - 3
            color: "#2d2d2d"
        }
    }

    Rectangle {
        objectName: "compassLead"
        readonly property real size: Math.max(6, 10 / tool.zoom)
        width: size
        height: size
        radius: size / 2
        x: tool.leadX - size / 2
        y: tool.leadY - size / 2
        color: tool.leadColor
        visible: tool.leadShown
    }

    // Zone de saisie : curseur et geste propres ; ne reçoit que les points de sa région du compas.
    component Zone: MathToolZone {
        anchors.fill: parent
        shaped: true
    }

    Zone { objectName: "compassMoveZone"; owner: tool; kind: "move"; cursorShape: Qt.SizeAllCursor }
    Zone { objectName: "compassRotateZone"; owner: tool; kind: "rotate"; cursorShape: Qt.CrossCursor }
    Zone { objectName: "compassRadiusZone"; owner: tool; kind: "radius"; cursorShape: Qt.SizeFDiagCursor }

    Text {
        objectName: "compassAngle"
        visible: tool.frame.showAngle === true
        x: tool.hingeX + 20
        y: tool.hingeY - height / 2
        text: (tool.frame.displayAngle || 0) + "°"
        font.pixelSize: 12
        color: "#1f2a35"
    }
    Text {
        objectName: "compassRadius"
        visible: tool.frame.showRadius === true
        x: tool.reach + tool.radius / 2 - width / 2
        y: tool.mirrored ? tool.reach - 8 - height : tool.reach + 8
        text: "r = " + (tool.frame.radiusText || "")
        font.pixelSize: 12
        color: "#1f2a35"
    }
    MathToolMenuButton {
        objectName: "compassMenuButton"
        menu: compassMenu
        x: tool.menuX - width / 2
        y: tool.menuY - height / 2
    }

    Menu {
        id: compassMenu
        objectName: "compassMenu"
        MenuItem {
            text: qsTr("Réfléchi")
            checkable: true
            checked: tool.mirrored
            onTriggered: if (tool.controller) tool.controller.setMirrored(tool.toolIndex, checked)
        }
        MenuItem {
            text: qsTr("Afficher l'angle")
            checkable: true
            checked: tool.frame.showAngle === true
            onTriggered: if (tool.controller) tool.controller.setShowAngle(tool.toolIndex, checked)
        }
        MenuItem {
            text: qsTr("Afficher le rayon")
            checkable: true
            checked: tool.frame.showRadius === true
            onTriggered: if (tool.controller) tool.controller.setShowRadius(tool.toolIndex, checked)
        }
        MenuItem {
            text: qsTr("Marquer le centre")
            checkable: true
            checked: tool.frame.markCentre === true
            onTriggered: if (tool.controller) tool.controller.setMarkCentre(tool.toolIndex, checked)
        }
        MenuSeparator {}
        MenuItem {
            text: qsTr("Augmenter la longueur")
            onTriggered: if (tool.controller) tool.controller.stepCompassLength(tool.toolIndex, 1)
        }
        MenuItem {
            text: qsTr("Réduire la longueur")
            onTriggered: if (tool.controller) tool.controller.stepCompassLength(tool.toolIndex, -1)
        }
        MenuItem {
            text: qsTr("Réinitialiser la longueur")
            onTriggered: if (tool.controller) tool.controller.stepCompassLength(tool.toolIndex, 0)
        }
        MenuSeparator {}
        MenuItem {
            text: qsTr("Fermer")
            onTriggered: tool.closeLater()
        }
    }

    function pagePoint(scenePoint) {
        return tool.parent ? tool.parent.mapFromItem(null, scenePoint.x, scenePoint.y) : scenePoint
    }
    function begin(kind, scenePress) {
        const p = pagePoint(scenePress)
        grab = { kind: kind, px: p.x, py: p.y, cx: frame.centerX, cy: frame.centerY, radius: radius,
                 angle: frame.angle }
    }
    function end() {
        grab = null
    }
    function drag(scenePoint) {
        const g = grab
        if (!g || !controller)
            return
        const p = pagePoint(scenePoint)
        if (g.kind === "move") {
            controller.setFrame(toolIndex, Qt.point(g.cx + p.x - g.px, g.cy + p.y - g.py), g.radius, leg, g.angle)
        } else if (g.kind === "rotate") {
            // Autour de la pointe, sans tracer : le bras suit le pointeur.
            const a0 = Math.atan2(g.py - g.cy, g.px - g.cx)
            const a1 = Math.atan2(p.y - g.cy, p.x - g.cx)
            let turn = (a1 - a0) * 180 / Math.PI
            if (turn > 180)
                turn -= 360
            else if (turn <= -180)
                turn += 360
            controller.setFrame(toolIndex, Qt.point(g.cx, g.cy), g.radius, leg, g.angle + turn)
        } else {
            // Rayon : la mine suit le pointeur, la pointe reste en place (bornes : contrôleur).
            const d0 = Math.hypot(g.px - g.cx, g.py - g.cy)
            const d1 = Math.hypot(p.x - g.cx, p.y - g.cy)
            controller.setFrame(toolIndex, Qt.point(g.cx, g.cy), g.radius + d1 - d0, leg, g.angle)
        }
    }
    // Fermé après le geste en cours : le délégué n'est pas détruit dans son propre gestionnaire.
    function closeLater() {
        const owner = tool.controller
        const index = tool.toolIndex
        if (owner)
            Qt.callLater(function () { owner.close(index) })
    }

    Component.onCompleted: if (pageView) pageView.addInputCover(tool)
    Component.onDestruction: if (pageView) pageView.removeInputCover(tool)
}
