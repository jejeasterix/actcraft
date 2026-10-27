import QtQuick
import ActCraft.Ui

// Éditeur de texte sur la page (F-250 à F-253, décision 37) : un TextEdit posé sur le cadre du texte en cours
// (appText.frame, coordonnées de page), à l'échelle du zoom. Le document du TextEdit est donné à TextEditing,
// qui y écrit les mises en forme et ne verse rien dans le paperboard avant la validation. Le TextEdit ne reçoit
// jamais de HTML d'ailleurs : TextEditing le charge, assaini (safeTextHtml). Sœur de la vue, au-dessus d'elle :
// la racine n'a aucun gestionnaire d'entrée (un appui hors du texte atteint l'outil, qui valide).
Item {
    id: root
    // Posés par PageArea (null pour une annotation de bureau : l'éditeur reste caché).
    property TextEditing editing: null
    property PageView pageView: null

    readonly property bool shown: editing !== null && editing.active
    readonly property real zoom: pageView ? pageView.zoom : 1
    readonly property color trait: "#1F4E9E" // cadre et poignées : plus de 3:1 sur la page blanche
    // Compteur réévalué quand la vue bouge : pageToScene n'annonce pas ses dépendances.
    property int tick: 0
    // Vrai pendant que la sélection du TextEdit et celle de TextEditing se recopient l'une dans l'autre.
    property bool syncing: false

    Connections {
        target: root.pageView
        function onZoomChanged() { root.tick++ }
        function onPanChanged() { root.tick++ }
        function onWidthChanged() { root.tick++ }
        function onHeightChanged() { root.tick++ }
    }

    // Point de page → coordonnées de cette surcouche.
    function place(point) {
        root.tick
        return root.mapFromItem(null, root.pageView.pageToScene(Qt.point(point.x, point.y)))
    }

    readonly property point origin: shown && pageView ? place(Qt.point(editing.frame.x, editing.frame.y)) : Qt.point(0, 0)

    // Document du TextEdit à TextEditing (il y écrit la mise en forme et en lit le HTML à la validation).
    Binding {
        target: root.editing
        property: "textDocument"
        value: textEdit.textDocument
        when: root.editing !== null
    }

    Connections {
        target: root.editing
        function onFocusRequested() { textEdit.forceActiveFocus() }
        // Sélection posée côté C++ (texte neuf tout sélectionné, « tout sélectionner », symbole inséré).
        function onFormatChanged() {
            if (root.syncing || !root.shown)
                return
            if (textEdit.selectionStart === root.editing.selectionStart
                    && textEdit.selectionEnd === root.editing.selectionEnd)
                return
            root.syncing = true
            textEdit.select(root.editing.selectionStart, root.editing.selectionEnd)
            root.syncing = false
        }
    }

    // Cadre : la zone du texte, en pointillés à nous (traits courts le long des quatre côtés).
    Item {
        id: box
        objectName: "textEditorFrame"
        visible: root.shown
        x: root.origin.x
        y: root.origin.y
        width: root.shown ? root.editing.frame.width * root.zoom : 0
        height: root.shown ? root.editing.frame.height * root.zoom : 0

        Item {
            id: dashes
            anchors.fill: parent
            anchors.margins: -3
            readonly property int pas: 8
            Repeater {
                model: Math.ceil(dashes.width / dashes.pas)
                delegate: Item {
                    required property int index
                    Rectangle { x: index * dashes.pas; y: 0; width: Math.min(4, dashes.width - x); height: 1; color: root.trait }
                    Rectangle { x: index * dashes.pas; y: dashes.height - 1; width: Math.min(4, dashes.width - x); height: 1; color: root.trait }
                }
            }
            Repeater {
                model: Math.ceil(dashes.height / dashes.pas)
                delegate: Item {
                    required property int index
                    Rectangle { x: 0; y: index * dashes.pas; width: 1; height: Math.min(4, dashes.height - y); color: root.trait }
                    Rectangle { x: dashes.width - 1; y: index * dashes.pas; width: 1; height: Math.min(4, dashes.height - y); color: root.trait }
                }
            }
        }

        TextEdit {
            id: textEdit
            objectName: "textEdit"
            x: 0
            y: 0
            width: root.shown ? Math.max(1, root.editing.frame.width) : 1
            scale: root.zoom
            transformOrigin: Item.TopLeft
            color: "#000000"
            wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
            selectByMouse: true
            persistentSelection: true // la sélection reste visible quand un bouton de la barre prend la souris
            cursorVisible: activeFocus

            onSelectionStartChanged: root.pushSelection()
            onSelectionEndChanged: root.pushSelection()

            // D8 : pendant la frappe, Ctrl+B, Ctrl+I et Ctrl+U sont à l'éditeur (le raccourci de commande,
            // navigateurs ou page vierge, ne part pas) ; D7 : Échap valide. Le raccourci global de l'outil de
            // sélection prendrait Échap d'abord : sans cette acceptation, ToolHost::cancelGesture n'atteindrait
            // jamais TextTool::cancel() et le texte ne serait pas validé.
            Keys.onShortcutOverride: event => {
                if (root.shown && root.ownedKey(event))
                    event.accepted = true
            }
            Keys.onPressed: event => {
                if (!root.shown || !root.ownedKey(event))
                    return
                event.accepted = true
                if (event.key === Qt.Key_Escape)
                    root.editing.commit()
                else
                    root.editing.apply(event.key === Qt.Key_B ? "text.bold"
                                       : event.key === Qt.Key_I ? "text.italic" : "text.underline", undefined)
            }
        }
    }

    // Touche prise par l'éditeur pendant l'édition : Échap, Ctrl+B, Ctrl+I, Ctrl+U (sans autre modificateur).
    function ownedKey(event) {
        if (event.key === Qt.Key_Escape)
            return true
        return event.modifiers === Qt.ControlModifier
                && (event.key === Qt.Key_B || event.key === Qt.Key_I || event.key === Qt.Key_U)
    }

    function pushSelection() {
        if (syncing || !shown)
            return
        syncing = true
        editing.selectionStart = textEdit.selectionStart
        editing.selectionEnd = textEdit.selectionEnd
        syncing = false
    }

    // Poignée de déplacement (F-250), hors du texte à son coin haut gauche.
    Rectangle {
        id: moveHandle
        objectName: "textMoveHandle"
        visible: root.shown
        width: 20
        height: 20
        radius: 4
        x: box.x - width - 6
        y: box.y - height - 6
        color: root.trait
        border.width: 1
        border.color: "#FFFFFF"
        Rectangle { anchors.centerIn: parent; width: 12; height: 2; color: "#FFFFFF" }
        Rectangle { anchors.centerIn: parent; width: 2; height: 12; color: "#FFFFFF" }
        DragHandler {
            id: moveDrag
            target: null
            property point last
            onActiveChanged: if (active) last = centroid.scenePosition
            onCentroidChanged: {
                if (!active)
                    return
                const now = centroid.scenePosition
                root.editing.moveFrame(Qt.point((now.x - last.x) / root.zoom, (now.y - last.y) / root.zoom))
                last = now
            }
        }
    }

    // Poignée d'extension de la largeur (F-250), au milieu du côté droit.
    Rectangle {
        id: widthHandle
        objectName: "textWidthHandle"
        visible: root.shown
        width: 12
        height: 28
        radius: 4
        x: box.x + box.width + 6
        y: box.y + box.height / 2 - height / 2
        color: root.trait
        border.width: 1
        border.color: "#FFFFFF"
        Rectangle { anchors.centerIn: parent; width: 2; height: 14; color: "#FFFFFF" }
        DragHandler {
            id: widthDrag
            target: null
            property real startX
            property real startWidth
            onActiveChanged: {
                if (active) {
                    startX = centroid.scenePosition.x
                    startWidth = root.editing.frame.width
                }
            }
            onCentroidChanged: {
                if (active)
                    root.editing.setFrameWidth(Math.max(20, startWidth + (centroid.scenePosition.x - startX) / root.zoom))
            }
        }
    }

    // A7 : l'éditeur et ses poignées prennent l'entrée (stylet, doigt, souris) : la vue ne la donne pas à l'outil.
    Component.onCompleted: {
        if (pageView) {
            pageView.addInputCover(box)
            pageView.addInputCover(moveHandle)
            pageView.addInputCover(widthHandle)
        }
    }
    Component.onDestruction: {
        if (pageView) {
            pageView.removeInputCover(box)
            pageView.removeInputCover(moveHandle)
            pageView.removeInputCover(widthHandle)
        }
    }
}
