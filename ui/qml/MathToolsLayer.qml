import QtQuick
import ActCraft.Ui

// Outils de maths (F-280 règle, F-283 équerre, F-281 rapporteur ; L7-T12, L7-T13 y ajoutent leurs délégués) :
// contrôleur, délégués en coordonnées de page (cette couche remplit pageOverlays), cercle d'accroche du survol,
// gestionnaires des commandes math.*. Les outils ne dessinent aucune encre : le stylo verse le trait, guidé par
// le filtre d'échantillons du contrôleur ; le rapporteur fermé ou plein fait ajouter ensuite ses rayons ou son
// secteur au document par le contrôleur.
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
        documentController: root.documentController
    }

    // Un modèle par outil, tenu par les signaux du contrôleur : ouvrir ou fermer un outil crée ou retire son seul
    // délégué (un Repeater sur le nombre recréerait tous les délégués, règles et équerres, à chaque changement).
    ListModel { id: slots }
    Connections {
        target: mathTools
        function onToolOpened(index, kind) { slots.insert(index, { kind: kind }) }
        function onToolClosed(index) { slots.remove(index) }
    }

    // Survol pris seulement près d'un bord guide, là où le cercle d'accroche a un sens : ailleurs, la couche
    // (z 30, toute la page) ne prend aucun survol, et les surcouches de dessous (spot) le reçoivent. Sous les
    // délégués : sur un outil (bande intérieure comprise), son survol et son curseur par zone passent d'abord.
    Item {
        id: snapArea
        anchors.fill: parent
        containmentMask: QtObject {
            function contains(point: point): bool {
                return mathTools.count > 0 && mathTools.snapAt(point).snapped === true
            }
        }
        HoverHandler {
            id: snapHover
            objectName: "mathToolSnapHover"
            acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus
            onPointChanged: root.updateSnap()
            onHoveredChanged: root.updateSnap()
        }

    Repeater {
        model: slots
        delegate: Loader {
            id: slot
            required property int index
            required property string kind
            sourceComponent: slot.kind === "setSquare" ? squareTool
                                                       : (slot.kind === "protractor" ? protractorTool : rulerTool)
            Component {
                id: rulerTool
                RulerTool {
                    controller: mathTools
                    toolIndex: slot.index
                    pageView: root.pageArea ? root.pageArea.view : null
                }
            }
            Component {
                id: squareTool
                SetSquareTool {
                    controller: mathTools
                    toolIndex: slot.index
                    pageView: root.pageArea ? root.pageArea.view : null
                }
            }
            Component {
                id: protractorTool
                ProtractorTool {
                    controller: mathTools
                    toolIndex: slot.index
                    pageView: root.pageArea ? root.pageArea.view : null
                }
            }
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
        root.commands.setHandler("math.set-square", function () { mathTools.openSetSquare() })
        root.commands.setHandler("math.protractor", function () { mathTools.openProtractor() })
    }
}
