import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Équerre (F-283), délégué de MathToolsLayer, en coordonnées de page : triangle rectangle isocèle dont le
// sommet de l'angle droit est l'origine de l'élément (côtés vers +x et +y, tournés par `rotation`). Corps
// translucide, graduations sur les deux côtés de l'angle droit, angle, menu. Elle ne dessine aucune encre et
// n'entre jamais au document. Le corps (le triangle, pas sa boîte) est une couverture de la vue : posé sur lui,
// tout pointeur saisit l'équerre ; le stylo se pose sur la bande extérieure d'un bord, et son trait y est guidé
// par le contrôleur. Zones : déplacer au centre, tourner aux deux pointes, redimensionner le long de
// l'hypoténuse.
Item {
    id: tool
    objectName: "setSquareTool"
    property QtObject controller: null
    property int toolIndex: -1
    property PageView pageView: null
    readonly property var frame: controller && controller.revision >= 0 ? controller.toolAt(toolIndex) : ({})
    readonly property real zoom: pageView ? Math.max(pageView.zoom, 0.01) : 1
    readonly property real rotateRadius: Math.min(60, width / 5)
    readonly property real scaleBand: Math.min(36, width / 8)
    property var grab: null

    x: frame.centerX || 0
    y: frame.centerY || 0
    width: frame.length || 0
    height: frame.length || 0
    rotation: frame.angle || 0
    transformOrigin: Item.TopLeft

    // Zone sous un point local ("" hors du triangle) : un seul prédicat pour la couverture et pour les gestes.
    function zoneAt(lx, ly) {
        const s = width
        if (lx < 0 || ly < 0 || lx + ly > s)
            return ""
        if (Math.hypot(lx - s, ly) < rotateRadius || Math.hypot(lx, ly - s) < rotateRadius)
            return "rotate"
        if ((s - lx - ly) / Math.SQRT2 < scaleBand)
            return "scale"
        return "move"
    }

    containmentMask: QtObject {
        function contains(point: point): bool {
            return tool.zoneAt(point.x, point.y) !== ""
        }
    }

    Canvas {
        id: body
        objectName: "setSquareBody"
        anchors.fill: parent
        renderStrategy: Canvas.Cooperative
        opacity: tool.controller ? tool.controller.defaultOpacity / 255 : 1
        onWidthChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.fillStyle = "#c3cfdb"
            ctx.beginPath()
            ctx.moveTo(0, 0)
            ctx.lineTo(width, 0)
            ctx.lineTo(0, height)
            ctx.closePath()
            ctx.fill()
        }
    }

    // Bords guides (filet pleine opacité) et graduations en centimètres sur les deux côtés de l'angle droit :
    // redessinés seulement quand le côté change.
    Canvas {
        id: scale
        objectName: "setSquareScale"
        anchors.fill: parent
        renderStrategy: Canvas.Cooperative
        onWidthChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.strokeStyle = "#6d7c8b"
            ctx.lineWidth = 1
            ctx.beginPath()
            ctx.moveTo(0, 0)
            ctx.lineTo(width, 0)
            ctx.lineTo(0, height)
            ctx.closePath()
            ctx.stroke()
            ctx.strokeStyle = "#33404d"
            ctx.fillStyle = "#33404d"
            ctx.font = "10px sans-serif"
            const perCm = 96 / 2.54
            const step = perCm / 10
            const n = Math.floor(width / step)
            ctx.beginPath()
            for (let i = 1; i <= n; ++i) {
                const t = i * step
                const major = i % 10 === 0
                const len = major ? 14 : (i % 5 === 0 ? 9 : 5)
                ctx.moveTo(t, 0)
                ctx.lineTo(t, len)
                ctx.moveTo(0, t)
                ctx.lineTo(len, t)
                if (major) {
                    ctx.fillText(String(i / 10), t + 2, 24)
                    ctx.fillText(String(i / 10), 18, t + 4)
                }
            }
            ctx.stroke()
        }
    }

    // Zone de saisie : curseur et geste propres ; ne reçoit que les points de sa région du triangle.
    component Zone: MathToolZone {
        anchors.fill: parent
        shaped: true
    }

    Zone { objectName: "setSquareMoveZone"; owner: tool; kind: "move"; cursorShape: Qt.SizeAllCursor }
    Zone { objectName: "setSquareRotateZone"; owner: tool; kind: "rotate"; cursorShape: Qt.CrossCursor }
    Zone { objectName: "setSquareScaleZone"; owner: tool; kind: "scale"; cursorShape: Qt.SizeFDiagCursor }

    Text {
        objectName: "setSquareAngle"
        visible: tool.frame.showAngle === true
        x: tool.width / 3 + 16
        y: tool.height / 3 - height / 2
        text: (tool.frame.displayAngle || 0) + "°"
        font.pixelSize: 12
        color: "#1f2a35"
    }
    MathToolMenuButton {
        objectName: "setSquareMenuButton"
        menu: squareMenu
        x: tool.width / 3 - width / 2
        y: tool.height / 3 - height / 2
    }

    Menu {
        id: squareMenu
        objectName: "setSquareMenu"
        MenuItem {
            text: qsTr("Afficher l'angle")
            checkable: true
            checked: tool.frame.showAngle === true
            onTriggered: if (tool.controller) tool.controller.setShowAngle(tool.toolIndex, checked)
        }
        MenuItem {
            text: qsTr("Fermer")
            onTriggered: if (tool.controller) tool.controller.close(tool.toolIndex)
        }
    }

    function pagePoint(scenePoint) {
        return tool.parent ? tool.parent.mapFromItem(null, scenePoint.x, scenePoint.y) : scenePoint
    }
    // Point de page dans le repère de l'équerre au début du geste (sommet fixe, angle de départ).
    function localAt(g, p) {
        const rad = g.angle * Math.PI / 180
        const dx = p.x - g.cx
        const dy = p.y - g.cy
        return Qt.point(dx * Math.cos(rad) + dy * Math.sin(rad), -dx * Math.sin(rad) + dy * Math.cos(rad))
    }
    function begin(kind, scenePress) {
        const p = pagePoint(scenePress)
        grab = { kind: kind, px: p.x, py: p.y, cx: frame.centerX, cy: frame.centerY, side: frame.length,
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
            controller.setFrame(toolIndex, Qt.point(g.cx + p.x - g.px, g.cy + p.y - g.py), g.side, g.side, g.angle)
        } else if (g.kind === "rotate") {
            // Autour du sommet de l'angle droit.
            const a0 = Math.atan2(g.py - g.cy, g.px - g.cx)
            const a1 = Math.atan2(p.y - g.cy, p.x - g.cx)
            controller.setFrame(toolIndex, Qt.point(g.cx, g.cy), g.side, g.side, g.angle + (a1 - a0) * 180 / Math.PI)
        } else {
            // Redimensionner : l'hypoténuse suit le pointeur, le sommet reste en place.
            const l0 = localAt(g, Qt.point(g.px, g.py))
            const l1 = localAt(g, p)
            const side = Math.min(controller.maximumLength, Math.max(controller.minimumLength,
                                                                      g.side + (l1.x + l1.y) - (l0.x + l0.y)))
            controller.setFrame(toolIndex, Qt.point(g.cx, g.cy), side, side, g.angle)
        }
    }

    Component.onCompleted: if (pageView) pageView.addInputCover(tool)
    Component.onDestruction: if (pageView) pageView.removeInputCover(tool)
}
