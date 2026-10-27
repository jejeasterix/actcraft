import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Navigateur de pages (F-800 à F-803, S7.4) : colonne de miniatures (PageThumbnails), page affichée encadrée,
// pages sélectionnées teintées. Clic : appPages.click ; glisser une sélection : fantôme d'insertion puis
// appPages.moveSelection ; clic droit : menu de page ; Ctrl+X, Ctrl+C, Ctrl+V, Ctrl+D et Suppr agissent sur les
// pages quand le navigateur a le focus ; Haut et Bas changent de page (avec Maj, ils étendent la sélection). Le
// navigateur ne modifie le document que par Pages. Délégués : pageThumb<n>, n à partir de 1.
Item {
    id: root
    required property PageThumbnails thumbnails
    required property Pages pages
    required property DocumentController documentController
    required property Commands commands
    readonly property int cellPad: 6
    readonly property int dragThreshold: 6

    clip: true
    activeFocusOnTab: true

    // La miniature de la page affichée est entièrement dans la vue (lu par le banc). Relue à chaque événement
    // de la liste (mesuré : une liaison déclarative sur itemAtIndex restait fausse à 9 pages, un calcul frais
    // donnait vrai) ; refreshVisible est reporté, donc regroupé.
    property bool visibleNow: false
    readonly property bool currentVisible: root.visibleNow
    function refreshVisible() {
        const item = list.itemAtIndex(root.documentController.currentIndex)
        root.visibleNow = root.visible && item !== null && item.y >= list.contentY - 0.5
                          && item.y + item.height <= list.contentY + list.height + 0.5
    }
    Connections {
        target: list
        function onContentYChanged() { root.refreshVisible() }
        function onContentHeightChanged() { root.refreshVisible() }
        function onHeightChanged() { root.refreshVisible() }
        function onCountChanged() { Qt.callLater(root.refreshVisible) }
    }

    // Tout appui sur la page rend le focus : Suppr, Ctrl+X, C, V et D ne valent que tant que le navigateur a le
    // focus (sinon Suppr supprimerait une page quand on veut supprimer l'objet sélectionné). Le clic droit ne
    // tient aucun contact : la vue le signale par contextMenuRequested.
    Connections {
        target: root.thumbnails ? root.thumbnails.view : null
        function onContactActiveChanged() {
            if (target.contactActive && root.activeFocus)
                root.focus = false
        }
        function onContextMenuRequested() {
            if (root.activeFocus)
                root.focus = false
        }
    }

    // La page affichée est amenée dans la vue quand elle change (page suivante, aller à, insertion).
    Connections {
        target: root.documentController
        function onPagesChanged() {
            Qt.callLater(root.showCurrent)
            Qt.callLater(root.refreshVisible)
        }
    }
    // Jamais de repositionnement de la liste sous un appui ou un défilement au doigt : la demande est gardée et
    // jouée au relâché (flushShow).
    property bool showPending: false
    function showCurrent() {
        if (area.pressed || touchScroll.active) {
            root.showPending = true
            return
        }
        root.showPending = false
        if (list.count > 0)
            list.positionViewAtIndex(Math.max(0, Math.min(list.count - 1, root.documentController.currentIndex)),
                                     ListView.Contain)
    }
    function flushShow() {
        if (root.showPending)
            Qt.callLater(root.showCurrent)
    }
    Component.onCompleted: Qt.callLater(root.showCurrent)
    onVisibleChanged: if (visible) Qt.callLater(root.showCurrent)

    // Un raccourci de la fenêtre (Suppr = supprimer des objets) ne passe pas avant ces touches.
    Keys.onShortcutOverride: event => {
        const ctrl = (event.modifiers & Qt.ControlModifier) !== 0
        if (event.key === Qt.Key_Delete
                || (ctrl && (event.key === Qt.Key_X || event.key === Qt.Key_C || event.key === Qt.Key_V
                             || event.key === Qt.Key_D)))
            event.accepted = true
    }
    Keys.onPressed: event => {
        const ctrl = (event.modifiers & Qt.ControlModifier) !== 0
        const shift = (event.modifiers & Qt.ShiftModifier) !== 0
        event.accepted = true
        if (event.key === Qt.Key_Delete)
            root.pages.deleteSelection()
        else if (ctrl && event.key === Qt.Key_X)
            root.pages.cutSelection()
        else if (ctrl && event.key === Qt.Key_C)
            root.pages.copySelection()
        else if (ctrl && event.key === Qt.Key_V)
            root.pages.paste()
        else if (ctrl && event.key === Qt.Key_D)
            root.pages.duplicateSelection()
        else if (event.key === Qt.Key_Down)
            root.step(1, shift)
        else if (event.key === Qt.Key_Up)
            root.step(-1, shift)
        else
            event.accepted = false
    }

    // Page voisine de la page affichée ; avec Maj, la plage s'étend jusqu'à elle.
    function step(delta, extend) {
        const count = root.documentController.pageCount
        const to = Math.max(0, Math.min(count - 1, root.documentController.currentIndex + delta))
        root.pages.click(to, extend ? Qt.ShiftModifier : 0)
    }

    // Menu du clic droit sur une miniature : celui de la page (contrat de F-202).
    PageContextMenu {
        id: menu
        objectName: "pageNavigatorMenu"
        commands: root.commands
    }
    // Menu propre au navigateur (icône de menu) : les commandes de pages seules.
    CommandMenu {
        id: ownMenu
        objectName: "pageNavigatorOwnMenu"
        commands: root.commands
        spec: ({ id: "", label: "", items: [
            { id: "page.insert-before" }, { id: "page.insert-after" }, { separator: true },
            { id: "page.cut" }, { id: "page.copy" }, { id: "page.paste" }, { separator: true },
            { id: "page.duplicate" }, { id: "page.delete" }] })
    }

    // Réglette de taille des miniatures et menu du navigateur.
    Row {
        id: header
        objectName: "navigatorHeader"
        x: Theme.espace2
        width: root.width - 2 * Theme.espace2
        height: 32
        spacing: Theme.espace1
        Slider {
            id: sizeSlider
            objectName: "thumbnailSize"
            width: header.width - menuButton.width - header.spacing
            height: header.height
            from: 80
            to: 320
            stepSize: 10
            value: root.thumbnails.thumbnailWidth
            onMoved: root.thumbnails.thumbnailWidth = Math.round(value)
            focusPolicy: Qt.NoFocus
            Accessible.name: qsTr("Taille des miniatures")
        }
        ToolButton {
            id: menuButton
            objectName: "navigatorMenuButton"
            width: header.height
            height: header.height
            display: AbstractButton.IconOnly
            focusPolicy: Qt.NoFocus
            icon.source: "qrc:/qt/qml/ActCraft/Ui/icons/menu.svg"
            icon.width: Theme.tailleIcone
            icon.height: Theme.tailleIcone
            icon.color: Theme.texte
            Accessible.name: qsTr("Menu des pages")
            background: Rectangle {
                radius: Theme.rayon
                color: menuButton.hovered ? Theme.surfaceSurvol : Theme.surface
            }
            onClicked: ownMenu.popup(menuButton.x + header.x, header.y + menuButton.height)
        }
    }

    // Zone d'appui : sous la liste (délégués, icône de menu de la page et barre de défilement passent avant),
    // et non dans son contenu : les coordonnées de la souris sont celles de la partie visible, converties une
    // seule fois en ordonnées du contenu (list.contentY).
    Item {
        id: pressZone
        anchors.fill: list

        // Au doigt, glisser fait défiler (le stylet et la souris déplacent des pages, la molette et la barre
        // font défiler aussi).
        DragHandler {
            id: touchScroll
            objectName: "pageListTouchScroll"
            target: null
            acceptedDevices: PointerDevice.TouchScreen
            xAxis.enabled: false
            property real startY: 0
            onActiveChanged: {
                if (active) {
                    startY = list.contentY
                    area.scrolled = true
                } else {
                    root.flushShow()
                }
            }
            onTranslationChanged: {
                if (!active)
                    return
                const low = list.originY
                const high = low + Math.max(0, list.contentHeight - list.height)
                list.contentY = Math.max(low, Math.min(high, startY - activeTranslation.y))
            }
        }

        MouseArea {
            id: area
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            property int pressedIndex: -1
            property real pressedY: 0
            property bool deferred: false
            property bool dragging: false
            property bool touch: false     // appui synthétisé depuis un doigt
            property bool scrolled: false  // le doigt a fait défiler la liste
            property int before: 0
            property real insertY: 0

            // Position d'insertion (indice « avant ») et ordonnée du trait pour une ordonnée de la partie visible.
            function locate(y) {
                const count = list.count
                const content = y + list.contentY
                for (let i = 0; i < count; ++i) {
                    const item = list.itemAtIndex(i)
                    if (!item)
                        continue
                    if (content < item.y + item.height / 2) {
                        area.before = i
                        area.insertY = item.y - list.spacing / 2
                        return
                    }
                }
                area.before = count
                const last = list.itemAtIndex(count - 1)
                area.insertY = last ? last.y + last.height + list.spacing / 2 : 0
            }

            onPressed: mouse => {
                root.forceActiveFocus()
                const index = list.indexAt(mouse.x, mouse.y + list.contentY)
                area.pressedIndex = index
                area.pressedY = mouse.y
                area.dragging = false
                area.deferred = false
                area.scrolled = false
                // mouse.source reste le seul moyen sûr : un PointHandler tactile n'est pas encore actif à l'arrivée de la
                // souris synthétisée (mesuré : le glisser au doigt cliquait alors la page sous le doigt).
                area.touch = mouse.source !== Qt.MouseEventNotSynthesized
                if (index < 0)
                    return
                if (mouse.button === Qt.RightButton) {
                    if (root.pages.selection.indexOf(index) < 0)
                        root.pages.click(index, 0)
                    menu.popupAt(mouse.x + list.x, mouse.y + list.y)
                    return
                }
                if (area.touch)
                    return // au doigt : un clic au relâché seulement, et sans défilement
                const plain = (mouse.modifiers & (Qt.ShiftModifier | Qt.ControlModifier)) === 0
                if (plain && root.pages.selection.length > 1 && root.pages.selection.indexOf(index) >= 0)
                    area.deferred = true // peut-être le début du glisser de toute la sélection
                else
                    root.pages.click(index, mouse.modifiers)
            }
            onPositionChanged: mouse => {
                if (area.touch || area.pressedIndex < 0 || !(mouse.buttons & Qt.LeftButton))
                    return
                if (!area.dragging && Math.abs(mouse.y - area.pressedY) > root.dragThreshold)
                    area.dragging = true
                if (area.dragging)
                    area.locate(mouse.y)
            }
            onReleased: mouse => {
                if (area.pressedIndex >= 0 && mouse.button === Qt.LeftButton) {
                    if (area.touch) {
                        if (!area.scrolled)
                            root.pages.click(area.pressedIndex, 0)
                    } else if (area.dragging) {
                        root.pages.moveSelection(area.before)
                    } else if (area.deferred) {
                        root.pages.click(area.pressedIndex, 0)
                    }
                }
                area.dragging = false
                area.deferred = false
                area.pressedIndex = -1
                root.flushShow()
            }
            onCanceled: {
                area.dragging = false
                area.deferred = false
                area.pressedIndex = -1
                root.flushShow()
            }
            onWheel: wheel => {
                const low = list.originY
                const high = low + Math.max(0, list.contentHeight - list.height)
                list.contentY = Math.max(low, Math.min(high, list.contentY - wheel.angleDelta.y / 2))
            }
        }
    }

    ListView {
        id: list
        objectName: "pageList"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.topMargin: Theme.espace1
        spacing: Theme.espace1
        clip: true
        model: root.thumbnails
        boundsBehavior: Flickable.StopAtBounds
        interactive: false // le glisser à la souris ou au stylet déplace des pages ; défilement : pressZone
        ScrollBar.vertical: ScrollBar {
            objectName: "pageListScrollBar"
            policy: ScrollBar.AsNeeded
        }

        delegate: Item {
            id: cell
            required property int index
            required property int pageNumber
            required property int revision
            required property real aspect
            required property bool current
            readonly property bool selected: root.pages.selection.indexOf(index) >= 0
            readonly property real shownWidth: Math.max(40, Math.min(root.thumbnails.thumbnailWidth,
                list.width - 2 * Theme.espace2 - 2 * root.cellPad))
            objectName: "pageThumb" + pageNumber
            width: list.width
            height: shownWidth * aspect + 2 * root.cellPad
            // Reporté : le délégué n'est dans la liste (itemAtIndex) qu'après sa création.
            Component.onCompleted: Qt.callLater(root.refreshVisible)
            onHeightChanged: Qt.callLater(root.refreshVisible)
            onYChanged: Qt.callLater(root.refreshVisible)

            Rectangle {
                x: Theme.espace2
                width: parent.width - 2 * Theme.espace2
                height: parent.height
                radius: 3
                color: cell.selected ? Theme.surfaceSurvol : "transparent"
                border.width: cell.current ? 3 : 0
                border.color: Theme.surfaceActive
            }
            ThumbnailImage {
                x: Math.round((cell.width - width) / 2)
                y: root.cellPad
                width: cell.shownWidth
                height: cell.shownWidth * cell.aspect
                source: root.thumbnails
                page: cell.index
                revision: cell.revision
            }
            Text {
                x: Theme.espace2 + 4
                y: parent.height - height - 4
                text: cell.pageNumber
                color: Theme.texte
                font.pixelSize: Theme.taillePolice
                style: Text.Outline
                styleColor: Theme.barre
            }
            // Page affichée : icône du menu de page.
            ToolButton {
                visible: cell.current
                objectName: "pageThumbMenu" + cell.pageNumber
                x: cell.width - width - Theme.espace2 - 2
                y: 2
                width: 24
                height: 24
                display: AbstractButton.IconOnly
                focusPolicy: Qt.NoFocus
                icon.source: "qrc:/qt/qml/ActCraft/Ui/icons/menu.svg"
                icon.width: 16
                icon.height: 16
                icon.color: Theme.texte
                Accessible.name: qsTr("Menu de la page")
                background: Rectangle {
                    radius: 3
                    color: Theme.barre
                    opacity: 0.8
                }
                // Menu de la page (pas celui du navigateur), sur la sélection ; une page hors sélection la remplace.
                onClicked: {
                    if (root.pages.selection.indexOf(cell.index) < 0)
                        root.pages.click(cell.index, 0)
                    menu.popupAt(x + cell.x + list.x, y + cell.y - list.contentY + list.y)
                }
            }
        }
    }

    // Fantôme d'insertion : trait entre deux miniatures pendant le glisser (au-dessus de la liste, sans entrée).
    Item {
        x: list.x
        y: list.y
        width: list.width
        height: list.height
        clip: true
        z: 2
        Rectangle {
            objectName: "insertionGhost"
            visible: area.dragging
            x: Theme.espace2
            width: parent.width - 2 * Theme.espace2
            height: 3
            radius: 1
            color: Theme.surfaceActive
            y: area.insertY - list.contentY - 1
        }
    }
}
