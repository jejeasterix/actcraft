import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Règle (F-280), délégué de MathToolsLayer, en coordonnées de page : corps translucide, graduations, zones de
// saisie (déplacer au centre, tourner à l'extrémité, allonger au bout), angle, menu. Elle ne dessine aucune
// encre et n'entre jamais au document. Le corps entier est une couverture de la vue : posé sur lui, tout pointeur
// saisit la règle ; le stylo se pose sur la bande extérieure d'un bord (moins de guideBand), et son trait y est
// guidé par le contrôleur.
Item {
    id: ruler
    objectName: "rulerTool"
    property QtObject controller: null
    property int toolIndex: -1
    property PageView pageView: null
    readonly property var frame: controller && controller.revision >= 0 ? controller.toolAt(toolIndex) : ({})
    readonly property real zoom: pageView ? Math.max(pageView.zoom, 0.01) : 1
    readonly property real tipLength: Math.min(24, width / 6)
    readonly property real endLength: Math.min(60, width / 5)
    property var grab: null

    x: (frame.centerX || 0) - width / 2
    y: (frame.centerY || 0) - height / 2
    width: frame.length || 0
    height: frame.thickness || 0
    rotation: frame.angle || 0
    transformOrigin: Item.Center

    Rectangle {
        id: body
        objectName: "rulerBody"
        anchors.fill: parent
        color: "#c3cfdb"
        opacity: ruler.controller ? ruler.controller.defaultOpacity / 255 : 1
    }
    // Bords guides : un filet fin, pleine opacité.
    Rectangle { width: parent.width; height: 1 / ruler.zoom; color: "#6d7c8b" }
    Rectangle { y: parent.height - height; width: parent.width; height: 1 / ruler.zoom; color: "#6d7c8b" }

    // Graduations : redessinées seulement quand la longueur ou une unité change.
    Canvas {
        id: scale
        anchors.fill: parent
        renderStrategy: Canvas.Cooperative
        property string upperUnit: ruler.frame.topUnit || "none"
        property string lowerUnit: ruler.frame.bottomUnit || "none"
        onUpperUnitChanged: requestPaint()
        onLowerUnitChanged: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.strokeStyle = "#33404d"
            ctx.fillStyle = "#33404d"
            ctx.lineWidth = 1
            ctx.font = "10px sans-serif"
            drawEdge(ctx, upperUnit, true)
            drawEdge(ctx, lowerUnit, false)
        }
        function drawEdge(ctx, unit, top) {
            if (unit === "none")
                return
            const inch = unit === "inch"
            const perUnit = inch ? 96 : 96 / 2.54 // pixels de page par pouce ou par centimètre
            const sub = inch ? 8 : 10             // huitièmes de pouce, millimètres
            const step = perUnit / sub
            const margin = 20
            const n = Math.floor((width - 2 * margin) / step)
            ctx.beginPath()
            for (let i = 0; i <= n; ++i) {
                const tx = margin + i * step
                const major = i % sub === 0
                const len = major ? 14 : (i % (sub / 2) === 0 ? 9 : 5)
                ctx.moveTo(tx, top ? 0 : height)
                ctx.lineTo(tx, top ? len : height - len)
                if (major) {
                    const label = unit === "mm" ? (i / sub) * 10 : i / sub
                    ctx.fillText(String(label), tx + 2, top ? 24 : height - 6)
                }
            }
            ctx.stroke()
        }
    }

    // Zone de saisie : curseur et geste propres, sur toute la largeur du corps.
    component Zone: MathToolZone {
        height: parent ? parent.height : 0
    }

    Zone { objectName: "rulerTipLeft"; owner: ruler; kind: "length-left"; cursorShape: Qt.SizeHorCursor
        x: 0; width: ruler.tipLength }
    Zone { objectName: "rulerEndLeft"; owner: ruler; kind: "rotate"; cursorShape: Qt.CrossCursor
        x: ruler.tipLength; width: ruler.endLength }
    Zone { objectName: "rulerMoveZone"; owner: ruler; kind: "move"; cursorShape: Qt.SizeAllCursor
        x: ruler.tipLength + ruler.endLength; width: Math.max(0, ruler.width - 2 * x) }
    Zone { objectName: "rulerEndRight"; owner: ruler; kind: "rotate"; cursorShape: Qt.CrossCursor
        x: ruler.width - ruler.tipLength - ruler.endLength; width: ruler.endLength }
    Zone { objectName: "rulerTipRight"; owner: ruler; kind: "length-right"; cursorShape: Qt.SizeHorCursor
        x: ruler.width - ruler.tipLength; width: ruler.tipLength }

    Text {
        objectName: "rulerAngle"
        visible: ruler.frame.showAngle === true
        x: parent.width / 2 + 16
        anchors.verticalCenter: parent.verticalCenter
        text: (ruler.frame.displayAngle || 0) + "°"
        font.pixelSize: 12
        color: "#1f2a35"
    }
    MathToolMenuButton {
        objectName: "rulerMenuButton"
        menu: rulerMenu
        x: (parent.width - width) / 2
        anchors.verticalCenter: parent.verticalCenter
    }

    Menu {
        id: rulerMenu
        objectName: "rulerMenu"
        Menu {
            title: qsTr("Unités du bord haut")
            MenuItem { text: qsTr("Centimètres"); onTriggered: ruler.setUnits("cm", "") }
            MenuItem { text: qsTr("Millimètres"); onTriggered: ruler.setUnits("mm", "") }
            MenuItem { text: qsTr("Pouces"); onTriggered: ruler.setUnits("inch", "") }
            MenuItem { text: qsTr("Aucune"); onTriggered: ruler.setUnits("none", "") }
        }
        Menu {
            title: qsTr("Unités du bord bas")
            MenuItem { text: qsTr("Centimètres"); onTriggered: ruler.setUnits("", "cm") }
            MenuItem { text: qsTr("Millimètres"); onTriggered: ruler.setUnits("", "mm") }
            MenuItem { text: qsTr("Pouces"); onTriggered: ruler.setUnits("", "inch") }
            MenuItem { text: qsTr("Aucune"); onTriggered: ruler.setUnits("", "none") }
        }
        MenuItem {
            text: qsTr("Afficher l'angle")
            checkable: true
            checked: ruler.frame.showAngle === true
            onTriggered: if (ruler.controller) ruler.controller.setShowAngle(ruler.toolIndex, checked)
        }
        MenuItem {
            text: qsTr("Fermer")
            onTriggered: if (ruler.controller) ruler.controller.close(ruler.toolIndex)
        }
    }

    function setUnits(top, bottom) {
        if (!controller)
            return
        controller.setUnits(toolIndex, top || frame.topUnit, bottom || frame.bottomUnit)
    }
    function pagePoint(scenePoint) {
        return ruler.parent ? ruler.parent.mapFromItem(null, scenePoint.x, scenePoint.y) : scenePoint
    }
    function begin(kind, scenePress) {
        const p = pagePoint(scenePress)
        grab = { kind: kind, px: p.x, py: p.y, cx: frame.centerX, cy: frame.centerY, length: frame.length,
                 thickness: frame.thickness, angle: frame.angle }
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
            controller.setFrame(toolIndex, Qt.point(g.cx + p.x - g.px, g.cy + p.y - g.py), g.length, g.thickness,
                                g.angle)
        } else if (g.kind === "rotate") {
            const a0 = Math.atan2(g.py - g.cy, g.px - g.cx)
            const a1 = Math.atan2(p.y - g.cy, p.x - g.cx)
            controller.setFrame(toolIndex, Qt.point(g.cx, g.cy), g.length, g.thickness,
                                g.angle + (a1 - a0) * 180 / Math.PI)
        } else {
            // Allonger : le bout opposé reste en place ; le point saisi suit le pointeur le long de la règle.
            const rad = g.angle * Math.PI / 180
            const ux = Math.cos(rad)
            const uy = Math.sin(rad)
            const sign = g.kind === "length-right" ? 1 : -1
            const fx = g.cx - sign * ux * g.length / 2
            const fy = g.cy - sign * uy * g.length / 2
            const along = sign * ((p.x - fx) * ux + (p.y - fy) * uy)
            const k = g.length - sign * ((g.px - fx) * ux + (g.py - fy) * uy)
            const length = Math.min(controller.maximumLength, Math.max(controller.minimumLength, along + k))
            controller.setFrame(toolIndex, Qt.point(fx + sign * ux * length / 2, fy + sign * uy * length / 2),
                                length, g.thickness, g.angle)
        }
    }

    Component.onCompleted: if (pageView) pageView.addInputCover(ruler)
    Component.onDestruction: if (pageView) pageView.removeInputCover(ruler)
}
