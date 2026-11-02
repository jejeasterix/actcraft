import QtQuick
import ActCraft.Ui

// Cadre de sélection, poignées, barre d'objet et rectangle élastique (S34, F-520 à F-528). Sœur de la vue, posée
// au-dessus d'elle. La géométrie vient de l'outil (appTools.selectionFrame, coordonnées de page) : ce fichier la
// place à l'écran, il ne la recalcule pas. Aucune entrée : pas de gestionnaire d'appui sur la racine (sinon le
// stylet hors du cadre ne tracerait plus), les poignées et les boutons sont désignés par l'outil.
Item {
    id: root

    required property Tools tools
    required property PageView view

    // Ouvre la réglette de translucidité (commande object.translucency sans argument).
    function openTranslucency() {
        translucency.open()
    }

    // Aspect : gris des poignées, contour sombre qui les détache de la page (15:1) et du fond (3,3:1).
    readonly property color gris: "#808080"
    readonly property color contour: "#101418"

    readonly property var frame: tools.selectionFrame
    readonly property bool framed: frame.visible === true
    readonly property bool handlesOn: tools.handlesVisible
    // Compteur réévalué quand la vue bouge : pageToScene n'annonce pas ses dépendances.
    property int tick: 0

    enabled: false // aucune entrée

    Connections {
        target: root.view
        function onZoomChanged() { root.tick++ }
        function onPanChanged() { root.tick++ }
        function onWidthChanged() { root.tick++ }
        function onHeightChanged() { root.tick++ }
    }

    // Point de page → coordonnées de cette surcouche.
    function place(point) {
        root.tick
        return root.mapFromItem(null, root.view.pageToScene(Qt.point(point.x, point.y)))
    }

    // Cadre : polygone des quatre coins, il pivote avec l'objet.
    Repeater {
        model: root.framed ? 4 : 0
        delegate: Rectangle {
            id: side
            objectName: "selectionSide"
            required property int index
            readonly property point from: root.place(root.frame.corners[index])
            readonly property point to: root.place(root.frame.corners[(index + 1) % 4])
            x: from.x
            y: from.y - height / 2
            width: Math.hypot(to.x - from.x, to.y - from.y)
            height: 2
            transformOrigin: Item.Left
            rotation: Math.atan2(to.y - from.y, to.x - from.x) * 180 / Math.PI
            color: root.contour
        }
    }

    // Poignées rondes ; la poignée bas droite (proportionnelle) est barrée d'un trait.
    Repeater {
        model: root.framed && root.handlesOn ? root.frame.handles : []
        delegate: Rectangle {
            id: handle
            objectName: "selectionHandle"
            required property int index
            required property var modelData
            readonly property point at: root.place(modelData)
            x: at.x - width / 2
            y: at.y - height / 2
            width: 12
            height: 12
            radius: 6
            color: root.gris
            border.width: 2
            border.color: root.contour
            Rectangle {
                visible: handle.index === 4
                anchors.centerIn: parent
                width: 20
                height: 2
                rotation: -45
                color: root.contour
            }
        }
    }

    // Points intelligents d'une forme sélectionnée seule (F-242) : losanges jaunes à contour sombre. Le tableau
    // vient de appTools.selectionFrame.smartPoints (vide tant que le cadre ne le porte pas).
    Repeater {
        model: root.framed && root.handlesOn ? (root.frame.smartPoints || []) : []
        delegate: Rectangle {
            id: smart
            objectName: "selectionSmartPoint"
            required property var modelData
            readonly property point at: root.place(modelData)
            x: at.x - width / 2
            y: at.y - height / 2
            width: 12
            height: 12
            rotation: 45
            color: "#FFD43B"
            border.width: 2
            border.color: root.contour
        }
    }

    // Rectangle élastique (glisser sur le vide).
    Rectangle {
        readonly property var band: root.frame.rubberBand
        readonly property point topLeft: band ? root.place(Qt.point(band.x, band.y)) : Qt.point(0, 0)
        readonly property point bottomRight: band ? root.place(Qt.point(band.x + band.width, band.y + band.height)) : Qt.point(0, 0)
        visible: !!band
        x: topLeft.x
        y: topLeft.y
        width: bottomRight.x - topLeft.x
        height: bottomRight.y - topLeft.y
        color: "#40808080"
        border.width: 1
        border.color: root.contour
    }

    ObjectBar {
        id: bar
        anchors.fill: parent
        tools: root.tools
        commands: root.tools.commands
        overlay: root
        visible: root.framed && root.handlesOn
    }

    TranslucencyPopup {
        id: translucency
        tools: root.tools
    }
}
