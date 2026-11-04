import QtQuick
import ActCraft.Ui

// Étiquette d'un objet (F-576, D12) : la légende s'affiche dans une bulle près du pointeur quand il reste 400 ms
// sur l'objet, souris ou stylet en survol. Le survol est lu par un HoverHandler passif, hors du routeur
// d'entrée : aucun trait, aucune étape d'annulation. La bulle est un élément QML au-dessus de la page : elle
// n'entre pas dans le cache.
Item {
    id: root
    objectName: "labelTip"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null

    readonly property string text: tips.text
    readonly property alias tipVisible: tips.visible
    readonly property int delay: 400
    property point pointer: Qt.point(0, 0)

    // Sans surface propre : la zone de survol et la bulle vivent dans la vue de page (un élément plein fenêtre au
    // dessus de la page prenait le survol des autres HoverHandler de PageArea).
    width: 0
    height: 0

    LabelTips {
        id: tips
        document: root.documentController
        tolerance: root.view && root.view.zoom > 0 ? 6 / root.view.zoom : 6
    }

    // Zone de survol : la zone de PageArea, parente de la vue. Un survol posé sur un élément voisin, au dessus
    // ou au dessous d'elle, prenait le survol des autres (ToolCursor, indicateur de largeur) ou ne recevait rien :
    // le gestionnaire est donc créé sur ce parent, comme ceux de PageArea.
    readonly property Item area: root.view ? root.view.parent : null
    property HoverHandler handler: null

    function attach() {
        if (root.handler) {
            root.handler.destroy()
            root.handler = null
        }
        if (root.area)
            root.handler = hoverComponent.createObject(root.area)
    }
    onAreaChanged: attach()
    Component.onCompleted: attach()

    // Point du pointeur (coordonnées de la zone) → coordonnées de page, ou null hors de la vue.
    function pagePointAt(p) {
        if (!root.view || !root.area)
            return null
        if (!root.view.contains(root.view.mapFromItem(root.area, p.x, p.y)))
            return null
        const scene = root.area.mapToItem(null, p.x, p.y)
        return root.view.sceneToPage(Qt.point(scene.x, scene.y))
    }

    function reveal() {
        if (root.view && root.view.contactActive) // un contact en cours : pas de bulle
            return
        const page = pagePointAt(root.pointer)
        if (page)
            tips.hoverAt(page)
    }

    function moved(next) {
        // Un événement qui ne bouge pas le pointeur (rafraîchissement de la scène) ne ferme pas la bulle.
        if (next.x === root.pointer.x && next.y === root.pointer.y)
            return
        root.pointer = next
        tips.leave()
        dwell.restart()
    }

    function pointerLeft() {
        dwell.stop()
        tips.leave()
    }

    Timer {
        id: dwell
        interval: root.delay
        onTriggered: root.reveal()
    }

    Component {
        id: hoverComponent
        HoverHandler {
            onPointChanged: root.moved(point.position)
            onHoveredChanged: if (!hovered) root.pointerLeft()
        }
    }

    // Bulle : un élément de la zone, sans gestionnaire, au dessus de la page (hors du cache).
    Rectangle {
        id: bubble
        parent: root.area
        z: 900
        visible: tips.visible && root.area !== null
        x: root.area ? Math.max(0, Math.min(root.area.width - width, root.pointer.x + 14)) : 0
        y: root.area ? Math.max(0, Math.min(root.area.height - height, root.pointer.y + 22)) : 0
        width: tipText.implicitWidth + 16
        height: tipText.implicitHeight + 10
        radius: 4
        color: tips.fill
        border.color: tips.outlineColor
        border.width: tips.outlineColor.a > 0 ? 1 : 0

        Text {
            id: tipText
            objectName: "labelTipText"
            anchors.centerIn: parent
            text: tips.text
            color: tips.color
            font.pointSize: tips.pointSize
            font.family: tips.fontFamily.length > 0 ? tips.fontFamily : Qt.application.font.family
        }
    }
}
