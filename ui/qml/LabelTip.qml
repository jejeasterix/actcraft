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

    anchors.fill: parent
    z: 900

    LabelTips {
        id: tips
        document: root.documentController
        tolerance: root.view && root.view.zoom > 0 ? 6 / root.view.zoom : 6
    }

    // Point du pointeur (coordonnées de cet élément) → coordonnées de page, ou null hors de la vue.
    function pagePointAt(p) {
        if (!root.view)
            return null
        const local = root.view.mapFromItem(root, p.x, p.y)
        if (!root.view.contains(local))
            return null
        const scene = root.mapToItem(null, p.x, p.y)
        return root.view.sceneToPage(Qt.point(scene.x, scene.y))
    }

    function reveal() {
        if (root.view && root.view.contactActive) // un contact en cours : pas de bulle
            return
        const page = pagePointAt(root.pointer)
        if (page)
            tips.hoverAt(page)
    }

    Timer {
        id: dwell
        interval: root.delay
        onTriggered: root.reveal()
    }

    HoverHandler {
        id: hover
        onPointChanged: {
            root.pointer = hover.point.position
            tips.leave()
            if (hover.hovered)
                dwell.restart()
        }
        onHoveredChanged: {
            if (!hover.hovered) {
                dwell.stop()
                tips.leave()
            }
        }
    }

    Rectangle {
        id: bubble
        visible: tips.visible
        x: Math.max(0, Math.min(root.width - width, root.pointer.x + 14))
        y: Math.max(0, Math.min(root.height - height, root.pointer.y + 22))
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
