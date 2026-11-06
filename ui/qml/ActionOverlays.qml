import QtQuick
// Surcouches des actions du lot 8 et gestionnaires des commandes qu'aucun autre fichier ne porte.
Item {
    id: overlays
    objectName: "actionOverlays"
    property var commands: null
    // Outils de l'application ; à défaut, ceux de la zone de page qui porte cette surcouche (lus à l'appel).
    property var tools: null

    function toolHost() {
        if (tools)
            return tools
        for (let p = parent; p; p = p.parent) {
            if (p.tools)
                return p.tools
        }
        return null
    }

    // L8-T9 : exécuteur d'actions (posé sur l'hôte des outils par Main.qml), mode conception et vue de page.
    readonly property var host: toolHost()
    readonly property var runner: host ? host.actionRunner : null
    readonly property var design: runner ? runner.designMode : null
    readonly property var view: host ? host.view : null
    // Cadre rouge à nous sur les objets d'action : effet « mettre en évidence » du mode conception (il suit
    // `active` ; highlightActions est le réglage seul).
    readonly property bool highlighted: design !== null && design.actionsHighlighted
    // Curseur d'action (F-890) : sous l'outil de sélection, pointeur sur un objet d'action.
    readonly property bool pointing: runner !== null && runner.hovering && host.currentTool === "select"
    // Hors de la scène que le cache photographie : rien n'est dessiné tant que la mise en évidence est éteinte.
    visible: highlighted

    function frameRect(frame) {
        if (!view)
            return Qt.rect(0, 0, 0, 0)
        // Dépendances : le cadre suit le zoom et le défilement de la vue.
        const zoom = view.zoom
        const pan = view.pan
        const a = overlays.mapFromItem(null, view.pageToScene(Qt.point(frame.x, frame.y)))
        const b = overlays.mapFromItem(null, view.pageToScene(Qt.point(frame.x + frame.width, frame.y + frame.height)))
        return Qt.rect(Math.min(a.x, b.x), Math.min(a.y, b.y), Math.abs(b.x - a.x), Math.abs(b.y - a.y))
    }

    Repeater {
        model: overlays.highlighted && overlays.runner ? overlays.runner.actionFrames : []
        delegate: Rectangle {
            required property var modelData
            readonly property rect box: overlays.frameRect(modelData)
            x: box.x - 2
            y: box.y - 2
            width: box.width + 4
            height: box.height + 4
            color: "transparent"
            border.color: "#d22a2a"
            border.width: 2
        }
    }

    // Survol passif, posé sur la zone qui porte la vue (comme LabelTip) : il ne prend aucune entrée.
    readonly property Item area: view ? view.parent : null
    property HoverHandler hoverHandler: null
    function attachHover() {
        if (hoverHandler) {
            hoverHandler.destroy()
            hoverHandler = null
        }
        if (area)
            hoverHandler = hoverComponent.createObject(area)
    }
    onAreaChanged: attachHover()
    Component {
        id: hoverComponent
        HoverHandler {
            cursorShape: overlays.pointing ? Qt.PointingHandCursor : undefined
            onPointChanged: {
                if (!overlays.runner || !overlays.view || !overlays.area)
                    return
                const scene = overlays.area.mapToItem(null, point.position.x, point.position.y)
                overlays.runner.hover(overlays.view.sceneToPage(Qt.point(scene.x, scene.y)))
            }
            onHoveredChanged: {
                if (!hovered && overlays.runner)
                    overlays.runner.hover(Qt.point(-1e9, -1e9))
            }
        }
    }

    Component.onCompleted: {
        attachHover()
        if (!commands)
            return
        // Lot 8 (L8-T18, F-255) : encre révélatrice.
        commands.setHandler("tool.reveal-ink", function () {
            const host = overlays.toolHost()
            if (host)
                host.activate("reveal-ink")
        })
        // page.history-back et page.history-forward : gestionnaires natifs de l'exécuteur d'actions (L8-T9).
    }
}
