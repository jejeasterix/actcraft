import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Cadre de la palette (F-030 à F-034) : ancrée à gauche, à droite, en haut, en bas ou flottante, réduite, réduite au
// maximum, épinglée ou repliée dans la marge. Tout vient de `profile` (persistance : ProfileController). Le cadre
// garde sa place dans la ligne de Main.qml (largeur `slotWidth`, nulle hors ancrage à droite) ; le panneau, lui, est
// placé dans le repère de cette ligne et se déclare couverture d'entrée de la page. Seuls `x` et `opacity` s'animent.
Item {
    id: root
    objectName: "paletteFrame"
    default property alias content: holder.data
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null

    readonly property string dock: root.profile ? root.profile.paletteDock : "right"
    readonly property bool pinned: root.profile ? root.profile.palettePinned : true
    readonly property bool collapsed: root.profile ? root.profile.paletteCollapsed : false
    readonly property bool floating: root.dock === "floating"
    readonly property bool horizontal: root.dock === "top" || root.dock === "bottom"
    // Repli au retrait du pointeur (désépinglée et ancrée) ; délai de D7.
    property bool folded: false
    property bool dragging: false
    property bool animateX: false // vrai le temps d'un repli ou d'un dépli : jamais au placement initial
    readonly property int foldDelayMs: 600
    readonly property int foldStrip: 8
    readonly property int titleHeight: 28
    // Place réservée dans la ligne : toute la largeur ancrée à droite et épinglée, la marge si elle est repliable.
    readonly property real slotWidth: root.dock !== "right" ? 0 : root.pinned ? Theme.largeurPalette : root.foldStrip

    readonly property Item region: root.parent
    readonly property real regionW: root.region ? root.region.width : 0
    readonly property real regionH: root.region ? root.region.height : 0
    readonly property var body: holder.children.length > 0 ? holder.children[0] : null
    readonly property real wantedHeight: root.body && root.body.contentHeight !== undefined ? root.body.contentHeight : 0
    readonly property real panelW: root.horizontal ? root.regionW : Theme.largeurPalette
    readonly property real panelH: {
        if (root.horizontal)
            return root.titleHeight + 2 * Theme.tailleBouton + 3 * Theme.espace2
        if (root.collapsed || root.floating)
            return Math.min(root.titleHeight + root.wantedHeight, root.floating ? 560 : root.regionH, root.regionH - 2 * Theme.espace2)
        return root.regionH
    }
    readonly property real floatX: root.profile ? root.profile.paletteFloat.x - (root.region ? root.region.x : 0) : 0
    readonly property real floatY: root.profile ? root.profile.paletteFloat.y - (root.region ? root.region.y : 0) : 0
    readonly property real homeX: root.dock === "right" ? root.regionW - root.panelW
        : root.floating ? Math.max(0, Math.min(root.floatX, root.regionW - root.panelW)) : 0
    readonly property real homeY: root.dock === "bottom" ? root.regionH - root.panelH
        : root.floating ? Math.max(0, Math.min(root.floatY, root.regionH - root.panelH)) : 0
    readonly property real shiftX: !root.folded ? 0
        : root.dock === "right" ? root.panelW - root.foldStrip
        : root.dock === "left" ? -(root.panelW - root.foldStrip) : 0
    readonly property real shiftY: !root.folded ? 0
        : root.dock === "bottom" ? root.panelH - root.foldStrip
        : root.dock === "top" ? -(root.panelH - root.foldStrip) : 0
    // Position du panneau pendant un glissement, dans le repère de la ligne.
    property real dragRX: 0
    property real dragRY: 0

    readonly property string idCollapse: "palette.collapse"
    readonly property string idCollapseFull: "palette.collapse-full"
    readonly property string idPin: "palette.pin"
    readonly property var optionsSpec: root.commands ? root.findSpec(root.commands.menus) : ({ id: "palette", label: "", items: [] })

    function findSpec(menus) {
        for (const menu of menus) {
            for (const entry of menu.items) {
                if (entry.submenu === true && entry.id === "palette")
                    return entry
            }
        }
        return ({ id: "palette", label: "", items: [] })
    }

    function setDock(name) {
        if (root.profile)
            root.profile.paletteDock = name
    }

    function syncChecks() {
        if (!root.commands || !root.profile)
            return
        root.commands.setChecked(root.idCollapse, root.profile.paletteCollapsed)
        root.commands.setChecked(root.idCollapseFull, root.profile.shortcutsHidden)
        root.commands.setChecked(root.idPin, root.profile.palettePinned)
    }

    // Repli : attendre le délai dès que le pointeur quitte le panneau, déplier dès qu'il y revient.
    readonly property bool autoFold: !root.pinned && !root.floating && !root.dragging
    function setFolded(value) {
        if (root.folded === value)
            return
        root.animateX = true
        root.folded = value
        animateReset.restart()
    }
    function updateFold() {
        if (!root.autoFold) {
            foldTimer.stop()
            root.setFolded(false)
        } else if (hover.hovered) {
            foldTimer.stop()
            root.setFolded(false)
        } else if (!root.folded) {
            foldTimer.restart()
        }
    }
    onAutoFoldChanged: root.updateFold()

    function finishDrag() {
        const l = root.dragRX
        const r = root.regionW - (root.dragRX + root.panelW)
        const t = root.dragRY
        const b = root.regionH - (root.dragRY + root.titleHeight)
        const nearest = Math.min(l, r, t, b)
        const scenePoint = Qt.point(root.dragRX + (root.region ? root.region.x : 0),
                                    root.dragRY + (root.region ? root.region.y : 0))
        if (root.profile) {
            if (nearest < 48) {
                root.profile.paletteDock = nearest === l ? "left" : nearest === r ? "right" : nearest === t ? "top" : "bottom"
            } else {
                root.profile.paletteFloat = scenePoint
                root.profile.paletteDock = "floating"
            }
        }
        root.dragging = false
    }

    // La taille des icônes de toute la fenêtre suit le profil (F-034) ; la garde de contact du profil suit la vue.
    Binding {
        target: Theme
        property: "tailleIcone"
        value: root.profile ? root.profile.iconPixels : 24
    }
    Binding {
        target: root.profile
        property: "contactActive"
        value: root.view ? root.view.contactActive : false
        when: root.profile !== null
    }
    Connections {
        target: root.profile
        function onProfileChanged() { root.syncChecks() }
    }

    Timer {
        id: foldTimer
        interval: root.foldDelayMs
        onTriggered: root.setFolded(true)
    }
    Timer {
        id: animateReset
        interval: 200
        onTriggered: root.animateX = false
    }

    Item {
        id: panel
        objectName: "palettePanel"
        x: root.dragging ? root.dragRX - root.x : root.homeX + root.shiftX - root.x
        y: root.dragging ? root.dragRY - root.y : root.homeY + root.shiftY - root.y
        width: root.panelW
        height: root.panelH
        // Seuls x et opacity s'animent (règle 4 de fluidité) ; jamais pendant un glissement.
        Behavior on x {
            enabled: root.animateX && !root.dragging
            NumberAnimation { duration: 120 }
        }

        HoverHandler {
            id: hover
            onHoveredChanged: root.updateFold()
        }

        Rectangle {
            anchors.fill: parent
            color: Theme.palette
            border.width: root.floating ? 1 : 0
            border.color: Theme.contour
        }

        Item {
            id: titleBar
            width: parent.width
            height: root.titleHeight

            Item {
                id: titleDrag
                objectName: "paletteTitle"
                anchors.fill: parent
                property real baseX: 0
                property real baseY: 0

                DragHandler {
                    id: dragHandler
                    target: null
                    acceptedButtons: Qt.LeftButton
                    function follow() {
                        const dx = dragHandler.centroid.scenePosition.x - dragHandler.centroid.scenePressPosition.x
                        const dy = dragHandler.centroid.scenePosition.y - dragHandler.centroid.scenePressPosition.y
                        root.dragRX = Math.max(-root.panelW / 2, Math.min(titleDrag.baseX + dx, root.regionW - root.panelW / 2))
                        root.dragRY = Math.max(0, Math.min(titleDrag.baseY + dy, root.regionH - root.titleHeight))
                        root.dragging = true
                    }
                    onActiveChanged: {
                        if (dragHandler.active) {
                            titleDrag.baseX = root.homeX + root.shiftX
                            titleDrag.baseY = root.homeY + root.shiftY
                            dragHandler.follow()
                        } else if (root.dragging) {
                            root.finishDrag()
                        }
                    }
                    onCentroidChanged: {
                        if (dragHandler.active)
                            dragHandler.follow()
                    }
                }
            }

            ToolButton {
                id: optionsButton
                objectName: "paletteOptions"
                x: Theme.espace1
                width: 24
                height: 24
                anchors.verticalCenter: parent.verticalCenter
                focusPolicy: Qt.NoFocus
                display: AbstractButton.IconOnly
                icon.source: root.commands ? root.commands.iconUrl("palette.float") : ""
                icon.width: 16
                icon.height: 16
                icon.color: Theme.texte
                onClicked: optionsMenu.popup(optionsButton, 0, optionsButton.height)
                CommandMenu {
                    id: optionsMenu
                    commands: root.commands
                    spec: root.optionsSpec
                }
            }
            ToolButton {
                id: pinButton
                objectName: "palettePinButton"
                anchors.right: parent.right
                anchors.rightMargin: Theme.espace1
                width: 24
                height: 24
                anchors.verticalCenter: parent.verticalCenter
                focusPolicy: Qt.NoFocus
                display: AbstractButton.IconOnly
                checkable: false
                checked: root.pinned
                icon.source: root.commands ? root.commands.iconUrl(root.idPin) : ""
                icon.width: 16
                icon.height: 16
                icon.color: root.pinned ? Theme.surfaceActive : Theme.texte
                onClicked: root.commands.trigger(root.idPin)
            }
            ToolButton {
                id: collapseButton
                objectName: "paletteCollapseButton"
                anchors.right: pinButton.left
                anchors.rightMargin: Theme.espace1
                width: 24
                height: 24
                anchors.verticalCenter: parent.verticalCenter
                focusPolicy: Qt.NoFocus
                display: AbstractButton.IconOnly
                icon.source: root.commands ? root.commands.iconUrl(root.idCollapse) : ""
                icon.width: 16
                icon.height: 16
                icon.color: root.collapsed ? Theme.surfaceActive : Theme.texte
                onClicked: root.commands.trigger(root.idCollapse)
            }
        }

        Item {
            id: holder
            anchors.top: titleBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
        }
    }

    Binding {
        target: root.body
        property: "horizontal"
        value: root.horizontal
        when: root.body !== null && root.body.horizontal !== undefined
    }

    Component.onCompleted: {
        if (root.view)
            root.view.addInputCover(panel)
        root.commands.setHandler("palette.dock-left", function () { root.setDock("left") })
        root.commands.setHandler("palette.dock-right", function () { root.setDock("right") })
        root.commands.setHandler("palette.dock-top", function () { root.setDock("top") })
        root.commands.setHandler("palette.dock-bottom", function () { root.setDock("bottom") })
        root.commands.setHandler("palette.float", function () { root.setDock("floating") })
        root.commands.setHandler("palette.collapse", function () {
            if (root.profile)
                root.profile.paletteCollapsed = !root.profile.paletteCollapsed
        })
        root.commands.setHandler("palette.collapse-full", function () {
            if (root.profile)
                root.profile.shortcutsHidden = !root.profile.shortcutsHidden
        })
        root.commands.setHandler("palette.pin", function () {
            if (root.profile)
                root.profile.palettePinned = !root.profile.palettePinned
        })
        root.commands.setHandler("palette.icon-size", function (pixels) {
            if (root.profile)
                root.profile.iconPixels = pixels
        })
        root.syncChecks()
        root.updateFold()
    }
}
