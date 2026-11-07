import QtQuick
import ActCraft.Ui
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
    // Hors de la scène que le cache photographie : les cadres ne sont dessinés que si la mise en évidence est
    // allumée, les icônes de glisser-déposer (L8-T13) toujours.
    // Modèle du glisser-déposer : posé par le parent, sinon cherché à la racine de la fenêtre (objectName
    // « appDragDrop »).
    property var dragDrop: null

    function findDragDrop() {
        let top = overlays
        while (top.parent)
            top = top.parent
        for (let i = 0; i < top.data.length; ++i) {
            if (top.data[i].objectName === "appDragDrop")
                return top.data[i]
        }
        return null
    }

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

    // Icônes de glisser-déposer en surimpression (F-892), au coin haut droit de l'objet. Glissée vers la corbeille,
    // l'icône retire l'action en une étape. Elle prend l'entrée là où elle est (couverture de la vue).
    Repeater {
        model: overlays.dragDrop ? overlays.dragDrop.overlays : []
        delegate: Rectangle {
            id: badge
            objectName: "dragDropBadge"
            required property var modelData
            readonly property rect box: overlays.frameRect(
                Qt.rect(modelData.x, modelData.y, modelData.width, modelData.height))
            width: 28
            height: 28
            radius: 6
            color: Theme.palette
            border.color: Theme.contour
            border.width: 1
            x: box.x + box.width - width - 2
            y: box.y + 2
            Accessible.name: modelData.label
            Image {
                anchors.centerIn: parent
                source: "qrc:/qt/qml/ActCraft/Ui/icons/" + badge.modelData.icon + ".svg"
                sourceSize.width: 20
                sourceSize.height: 20
                width: 20
                height: 20
            }
            Rectangle { // copie qui suit le pointeur pendant le glisser
                id: ghost
                width: badge.width
                height: badge.height
                radius: badge.radius
                visible: dragger.active
                color: badge.color
                border.color: Theme.surfaceActive
                border.width: 2
                Image {
                    anchors.centerIn: parent
                    source: "qrc:/qt/qml/ActCraft/Ui/icons/" + badge.modelData.icon + ".svg"
                    sourceSize.width: 20
                    sourceSize.height: 20
                    width: 20
                    height: 20
                }
            }
            DragHandler {
                id: dragger
                target: ghost
                onActiveChanged: {
                    if (active)
                        return
                    const scene = ghost.mapToItem(null, ghost.width / 2, ghost.height / 2)
                    ghost.x = 0
                    ghost.y = 0
                    const trash = overlays.host ? overlays.host.trashArea : Qt.rect(0, 0, 0, 0)
                    // QRectF n'a pas de méthode contains() en QML : comparaison des bornes.
                    if (trash.width > 0 && scene.x >= trash.x && scene.x <= trash.x + trash.width
                            && scene.y >= trash.y && scene.y <= trash.y + trash.height)
                        overlays.dragDrop.dropOnTrash(badge.modelData.objectId)
                }
            }
            Component.onCompleted: if (overlays.view) overlays.view.addInputCover(badge)
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
        if (!dragDrop)
            dragDrop = findDragDrop()
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
