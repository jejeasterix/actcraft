import QtQuick
import ActCraft.Ui

// Outils de maths (F-280 ; L7-T10 à L7-T13 y ajoutent leurs délégués) : contrôleur, délégués en coordonnées de
// page (cette couche remplit pageOverlays), cercle d'accroche du survol, gestionnaires des commandes math.*. Les
// outils ne dessinent aucune encre : le stylo verse le trait, guidé par le filtre d'échantillons du contrôleur.
Item {
    id: root
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property Item pageArea: null
    property QtObject controller: mathTools
    anchors.fill: parent

    MathToolsController {
        id: mathTools
        objectName: "mathTools"
        view: root.pageArea ? root.pageArea.view : null
        toolHost: root.tools
    }

    Repeater {
        model: mathTools.count
        delegate: RulerTool {
            required property int index
            controller: mathTools
            toolIndex: index
            pageView: root.pageArea ? root.pageArea.view : null
        }
    }

    // Cercle d'accroche : là où le stylo posé accrocherait le trait (même requête que le filtre, sans état).
    Rectangle {
        id: snapMarker
        objectName: "mathToolSnap"
        property real zoom: root.pageArea && root.pageArea.view ? root.pageArea.view.zoom : 1
        width: 12 / Math.max(zoom, 0.01)
        height: width
        radius: width / 2
        color: "transparent"
        border.color: "#2f6fbf"
        border.width: 2 / Math.max(zoom, 0.01)
        visible: false
    }
    HoverHandler {
        id: snapHover
        acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus
        onPointChanged: root.updateSnap()
        onHoveredChanged: root.updateSnap()
    }

    function updateSnap() {
        if (!snapHover.hovered || mathTools.count === 0) {
            snapMarker.visible = false
            return
        }
        const hit = mathTools.snapAt(snapHover.point.position)
        snapMarker.visible = hit.snapped
        if (hit.snapped) {
            snapMarker.x = hit.x - snapMarker.width / 2
            snapMarker.y = hit.y - snapMarker.height / 2
        }
    }

    Component.onCompleted: {
        if (!root.commands)
            return
        root.commands.setHandler("math.ruler", function () { mathTools.openRuler() })
    }
}
