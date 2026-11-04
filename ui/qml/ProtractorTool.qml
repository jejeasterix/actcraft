import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Templates as T
import ActCraft.Ui

// Rapporteur (F-281), délégué de MathToolsLayer, en coordonnées de page : disque (complet) ou demi-disque
// (au-dessus du diamètre à l'angle 0) translucide, centré sur le centre du cercle et tourné autour de lui.
// Graduations en degrés, angle balayé par le trait guidé, poignée de rotation, angle, menu. Il ne dessine aucune
// encre : le stylo posé sur la bande extérieure du pourtour verse un arc guidé par le contrôleur, qui ajoute
// ensuite au document, en style fermé ou plein, les deux rayons ou le secteur. Le corps (le disque, pas sa
// boîte) est une couverture de la vue : posé sur lui, tout pointeur saisit le rapporteur. Zones : tourner par la
// poignée, changer le rayon le long du pourtour intérieur, déplacer ailleurs.
Item {
    id: tool
    objectName: "protractorTool"
    property QtObject controller: null
    property int toolIndex: -1
    property PageView pageView: null
    readonly property var frame: controller && controller.revision >= 0 ? controller.toolAt(toolIndex) : ({})
    readonly property real radius: frame.length || 0
    readonly property bool half: frame.shape === "half"
    // Poignée de rotation : sur l'axe vertical, près du pourtour (en haut à l'angle 0, dans les deux formes).
    readonly property real handleDistance: radius - Math.min(44, radius / 4)
    readonly property real handleRadius: Math.min(18, radius / 6)
    readonly property real scaleBand: Math.min(20, radius / 8)
    property var grab: null

    x: (frame.centerX || 0) - radius
    y: (frame.centerY || 0) - radius
    width: 2 * radius
    height: 2 * radius
    rotation: frame.angle || 0
    transformOrigin: Item.Center

    // Zone sous un point local ("" hors du disque ou du demi-disque) : un seul prédicat pour la couverture et
    // pour les gestes.
    function zoneAt(lx, ly) {
        const dx = lx - radius
        const dy = ly - radius
        const d = Math.hypot(dx, dy)
        if (d > radius || (half && dy > 0))
            return ""
        if (Math.hypot(dx, dy + handleDistance) < handleRadius)
            return "rotate"
        if (d > radius - scaleBand)
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
        objectName: "protractorBody"
        anchors.fill: parent
        renderStrategy: Canvas.Cooperative
        opacity: tool.controller ? tool.controller.defaultOpacity / 255 : 1
        property bool half: tool.half
        onHalfChanged: requestPaint()
        onWidthChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            const r = width / 2
            ctx.fillStyle = "#c3cfdb"
            ctx.beginPath()
            if (half)
                ctx.arc(r, r, r, Math.PI, 2 * Math.PI, false)
            else
                ctx.arc(r, r, r, 0, 2 * Math.PI, false)
            ctx.closePath()
            ctx.fill()
        }
    }

    // Pourtour (filet pleine opacité) et graduations en degrés, comptés en sens trigonométrique depuis la
    // droite : redessinés seulement quand le rayon ou la forme change.
    Canvas {
        id: dial
        objectName: "protractorScale"
        anchors.fill: parent
        renderStrategy: Canvas.Cooperative
        property bool half: tool.half
        onHalfChanged: requestPaint()
        onWidthChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            const r = width / 2
            ctx.strokeStyle = "#6d7c8b"
            ctx.lineWidth = 1
            ctx.beginPath()
            if (half) {
                ctx.arc(r, r, r, Math.PI, 2 * Math.PI, false)
                ctx.closePath()
            } else {
                ctx.arc(r, r, r, 0, 2 * Math.PI, false)
            }
            ctx.stroke()
            ctx.strokeStyle = "#33404d"
            ctx.fillStyle = "#33404d"
            ctx.font = "10px sans-serif"
            ctx.beginPath()
            const last = half ? 180 : 359
            for (let a = 0; a <= last; ++a) {
                const t = -a * Math.PI / 180
                const len = a % 10 === 0 ? 14 : (a % 5 === 0 ? 9 : 5)
                ctx.moveTo(r + r * Math.cos(t), r + r * Math.sin(t))
                ctx.lineTo(r + (r - len) * Math.cos(t), r + (r - len) * Math.sin(t))
                if (a % 30 === 0)
                    ctx.fillText(String(a), r + (r - 28) * Math.cos(t) - 8, r + (r - 28) * Math.sin(t) + 4)
            }
            // Croix du centre.
            ctx.moveTo(r - 6, r)
            ctx.lineTo(r + 6, r)
            ctx.moveTo(r, r - 6)
            ctx.lineTo(r, r + 6)
            ctx.stroke()
        }
    }

    Rectangle {
        objectName: "protractorHandle"
        width: 2 * tool.handleRadius
        height: width
        radius: width / 2
        x: tool.radius - width / 2
        y: tool.radius - tool.handleDistance - height / 2
        color: "#f4f7fa"
        border.color: "#6d7c8b"
    }

    // Zone de saisie : curseur et geste propres ; ne reçoit que les points de sa région du disque.
    component Zone: Item {
        id: zone
        property var owner: null
        property string kind: "move"
        property int cursorShape: Qt.SizeAllCursor
        anchors.fill: parent
        containmentMask: QtObject {
            function contains(point: point): bool {
                return zone.owner !== null && zone.owner.zoneAt(point.x, point.y) === zone.kind
            }
        }
        HoverHandler { cursorShape: zone.cursorShape }
        DragHandler {
            target: null
            acceptedDevices: PointerDevice.AllDevices
            onActiveChanged: {
                if (!zone.owner)
                    return
                if (active)
                    zone.owner.begin(zone.kind, centroid.scenePressPosition)
                else
                    zone.owner.end()
            }
            onCentroidChanged: if (active && zone.owner) zone.owner.drag(centroid.scenePosition)
        }
    }

    Zone { objectName: "protractorMoveZone"; owner: tool; kind: "move"; cursorShape: Qt.SizeAllCursor }
    Zone { objectName: "protractorRotateZone"; owner: tool; kind: "rotate"; cursorShape: Qt.CrossCursor }
    Zone { objectName: "protractorScaleZone"; owner: tool; kind: "scale"; cursorShape: Qt.SizeFDiagCursor }

    // Angle balayé par le trait guidé, lu sur l'échelle.
    Text {
        objectName: "protractorSwept"
        visible: (tool.frame.swept || 0) > 0
        x: tool.radius - width / 2
        y: tool.radius - tool.radius / 2 - height / 2
        text: (tool.frame.swept || 0) + "°"
        font.pixelSize: 14
        font.bold: true
        color: "#1f2a35"
    }
    Text {
        objectName: "protractorAngle"
        visible: tool.frame.showAngle === true
        x: tool.radius + 16
        y: tool.radius - tool.radius / 4 - height / 2
        text: (tool.frame.displayAngle || 0) + "°"
        font.pixelSize: 12
        color: "#1f2a35"
    }
    Rectangle {
        objectName: "protractorMenuButton"
        width: 18
        height: 18
        radius: 4
        x: tool.radius - width / 2
        y: tool.radius - tool.radius / 4 - height / 2
        color: "#f4f7fa"
        border.color: "#6d7c8b"
        Text { anchors.centerIn: parent; text: "≡"; font.pixelSize: 12; color: "#1f2a35" }
        TapHandler { onTapped: protractorMenu.popup() }
    }

    // Coches exclusives : recliquer l'entrée cochée la laisse cochée. Sans groupe, le clic basculait `checked`
    // et seule une liaison réévaluée le rétablissait : jamais pour la forme (`tool.half` ne change pas).
    T.ButtonGroup { id: shapeGroup }
    T.ButtonGroup { id: arcGroup }

    Menu {
        id: protractorMenu
        objectName: "protractorMenu"
        MenuItem {
            text: qsTr("Rapporteur complet")
            checkable: true
            T.ButtonGroup.group: shapeGroup
            checked: !tool.half
            onTriggered: if (tool.controller) tool.controller.setProtractorShape(tool.toolIndex, "full")
        }
        MenuItem {
            text: qsTr("Demi-rapporteur")
            checkable: true
            T.ButtonGroup.group: shapeGroup
            checked: tool.half
            onTriggered: if (tool.controller) tool.controller.setProtractorShape(tool.toolIndex, "half")
        }
        MenuSeparator {}
        MenuItem {
            text: qsTr("Arc")
            checkable: true
            T.ButtonGroup.group: arcGroup
            checked: tool.frame.arcStyle === "arc"
            onTriggered: if (tool.controller) tool.controller.setArcStyle(tool.toolIndex, "arc")
        }
        MenuItem {
            text: qsTr("Arc fermé")
            checkable: true
            T.ButtonGroup.group: arcGroup
            checked: tool.frame.arcStyle === "closed"
            onTriggered: if (tool.controller) tool.controller.setArcStyle(tool.toolIndex, "closed")
        }
        MenuItem {
            text: qsTr("Arc plein")
            checkable: true
            T.ButtonGroup.group: arcGroup
            checked: tool.frame.arcStyle === "filled"
            onTriggered: if (tool.controller) tool.controller.setArcStyle(tool.toolIndex, "filled")
        }
        MenuSeparator {}
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
            controller.setFrame(toolIndex, Qt.point(g.cx + p.x - g.px, g.cy + p.y - g.py), g.radius, g.radius,
                                g.angle)
        } else if (g.kind === "rotate") {
            // Autour du centre : la poignée suit le pointeur.
            const a0 = Math.atan2(g.py - g.cy, g.px - g.cx)
            const a1 = Math.atan2(p.y - g.cy, p.x - g.cx)
            let turn = (a1 - a0) * 180 / Math.PI
            if (turn > 180)
                turn -= 360
            else if (turn <= -180)
                turn += 360
            controller.setFrame(toolIndex, Qt.point(g.cx, g.cy), g.radius, g.radius, g.angle + turn)
        } else {
            // Rayon : le pourtour suit le pointeur, le centre reste en place.
            const d0 = Math.hypot(g.px - g.cx, g.py - g.cy)
            const d1 = Math.hypot(p.x - g.cx, p.y - g.cy)
            const r = Math.min(controller.maximumRadius, Math.max(controller.minimumRadius, g.radius + d1 - d0))
            controller.setFrame(toolIndex, Qt.point(g.cx, g.cy), r, r, g.angle)
        }
    }

    Component.onCompleted: if (pageView) pageView.addInputCover(tool)
    Component.onDestruction: if (pageView) pageView.removeInputCover(tool)
}
