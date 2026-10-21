import QtQuick
import ActCraft.Ui

// Origine XY (F-284), marqueur de MathToolsLayer, en coordonnées de page : croix et cercle translucides sur
// l'origine de la page affichée (controller.origin). Visible tant que le contrôleur le demande
// (originVisible). Déplaçable : pendant le geste il suit le pointeur sans rien écrire ; au relâché, une seule
// écriture dans la présentation de la page (controller.moveOrigin, une étape d'annulation). Le disque est une
// couverture de la vue : posé dessus, tout pointeur le déplace et ne verse aucune encre.
Item {
    id: marker
    property QtObject controller: null
    property PageView pageView: null
    // Position suivie pendant le geste (nulle hors geste).
    property var live: null
    property var grab: null
    readonly property real zoom: pageView ? Math.max(pageView.zoom, 0.01) : 1
    readonly property point shown: live ? Qt.point(live.x, live.y)
                                        : (controller ? controller.origin : Qt.point(0, 0))

    visible: controller ? controller.originVisible : false
    width: 44 / zoom
    height: width
    x: shown.x - width / 2
    y: shown.y - height / 2

    containmentMask: QtObject {
        function contains(point: point): bool {
            return Math.hypot(point.x - marker.width / 2, point.y - marker.height / 2) <= marker.width / 2
        }
    }

    Rectangle {
        objectName: "originMarkerBody"
        anchors.fill: parent
        radius: width / 2
        color: "#c3cfdb"
        border.color: "#6d7c8b"
        border.width: 1 / marker.zoom
        opacity: marker.controller ? marker.controller.defaultOpacity / 255 : 1
    }
    // Croix de l'origine, pleine opacité.
    Rectangle {
        objectName: "originMarkerCross"
        x: 0
        y: marker.height / 2 - height / 2
        width: marker.width
        height: 2 / marker.zoom
        color: "#33404d"
    }
    Rectangle {
        x: marker.width / 2 - width / 2
        y: 0
        width: 2 / marker.zoom
        height: marker.height
        color: "#33404d"
    }

    HoverHandler { cursorShape: Qt.SizeAllCursor }
    DragHandler {
        target: null
        acceptedDevices: PointerDevice.AllDevices
        onActiveChanged: {
            if (active) {
                const p = marker.pagePoint(centroid.scenePressPosition)
                marker.grab = { px: p.x, py: p.y, ox: marker.shown.x, oy: marker.shown.y }
            } else {
                const target = marker.live
                marker.live = null
                marker.grab = null
                if (target && marker.controller)
                    marker.controller.moveOrigin(Qt.point(target.x, target.y))
            }
        }
        onCentroidChanged: {
            const g = marker.grab
            if (!active || !g)
                return
            const p = marker.pagePoint(centroid.scenePosition)
            marker.live = { x: g.ox + p.x - g.px, y: g.oy + p.y - g.py }
        }
    }

    function pagePoint(scenePoint) {
        return marker.parent ? marker.parent.mapFromItem(null, scenePoint.x, scenePoint.y) : scenePoint
    }

    Component.onCompleted: if (pageView) pageView.addInputCover(marker)
    Component.onDestruction: if (pageView) pageView.removeInputCover(marker)
}
