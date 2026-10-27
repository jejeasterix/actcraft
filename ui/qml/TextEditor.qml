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
    // Une liste de la barre de mise en forme est ouverte : Échap la ferme, il ne valide pas le texte.
    property bool listOpen: false

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

    // De haut en bas (L4-C17) : document de la colonne montrée, que TextEditing remplit à chaque frappe.
    Binding {
        target: root.editing
        property: "columnDocument"
        value: column.textDocument
        when: root.editing !== null
    }

    Connections {
        target: root.editing
        function onColumnChanged() { root.syncColumn() }
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

        // Un seul élément, redessiné quand le cadre change (Canvas : aucun module de plus que QtQuick ; un
        // Repeater de traits recréait tous ses délégués à chaque frappe et à chaque pas de zoom).
        Canvas {
            id: dashes
            anchors.fill: parent
            anchors.margins: -3
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()
                ctx.strokeStyle = root.trait
                ctx.lineWidth = 1
                ctx.setLineDash([4, 4])
                ctx.strokeRect(0.5, 0.5, width - 1, height - 1)
            }
        }

        TextEdit {
            id: textEdit
            objectName: "textEdit"
            // De haut en bas : largeur de repli = plus petite avance d'un graphème (une lettre par ligne, comme le
            // résultat validé), recentrée dans la colonne.
            x: root.shown && root.editing.vertical ? (box.width - width * root.zoom) / 2 : 0
            y: 0
            width: root.shown ? Math.max(1, root.editing.vertical ? root.editing.wrapWidth : root.editing.frame.width) : 1
            scale: root.zoom
            transformOrigin: Item.TopLeft
            color: "#000000"
            // De haut en bas : transparent, il garde l'entrée ; la colonne ci-dessous montre le texte (L4-C17).
            opacity: root.shown && root.editing.vertical ? 0 : 1
            // Texte brut : un collage ne fait jamais entrer de HTML (règle du chantier : jamais de texte riche
            // sans HTML assaini). La mise en forme vient de TextEditing, qui charge le HTML assaini.
            textFormat: TextEdit.PlainText
            // De haut en bas : colonne d'un caractère, lettres centrées, repli partout.
            horizontalAlignment: root.shown && root.editing.vertical ? TextEdit.AlignHCenter : TextEdit.AlignLeft
            wrapMode: root.shown && root.editing.vertical ? TextEdit.WrapAnywhere
                      : root.shown && root.editing.fixedWidth ? TextEdit.WrapAtWordBoundaryOrAnywhere : TextEdit.NoWrap
            selectByMouse: true
            persistentSelection: true // la sélection reste visible quand un bouton de la barre prend la souris
            cursorVisible: activeFocus

            onSelectionStartChanged: { root.pushSelection(); root.syncColumn() }
            onSelectionEndChanged: { root.pushSelection(); root.syncColumn() }
            onCursorPositionChanged: root.syncColumn()

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

        // De haut en bas (L4-C17) : la colonne montrée, copie du texte mise en page par le peintre
        // (TextEditing.columnDocument) : une ligne par graphème, espaces compris, comme le résultat validé. Qt
        // laisse un espace pendre au bout de la ligne de la lettre précédente : le TextEdit d'entrée ne peut pas
        // le montrer, il reste transparent et garde le texte, le curseur, la sélection et l'annulation. La
        // colonne en montre le curseur et la sélection ; un appui y place le curseur du TextEdit d'entrée.
        TextEdit {
            id: column
            objectName: "textColumn"
            visible: root.shown && root.editing.vertical
            width: root.shown ? Math.max(1, root.editing.frame.width) : 1
            scale: root.zoom
            transformOrigin: Item.TopLeft
            color: textEdit.color
            textFormat: TextEdit.PlainText // le contenu vient de TextEditing, jamais d'un texte riche
            readOnly: true
            activeFocusOnPress: false
            selectByMouse: false
            persistentSelection: true
            wrapMode: TextEdit.WrapAnywhere
            horizontalAlignment: TextEdit.AlignHCenter
            onWidthChanged: root.syncColumn()

            Rectangle {
                id: caret
                objectName: "textColumnCaret"
                property rect at: Qt.rect(0, 0, 0, 0)
                visible: column.visible && textEdit.activeFocus
                x: at.x
                y: at.y
                width: 1
                height: at.height
                color: textEdit.color
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.IBeamCursor
                function editAt(mouse) {
                    return root.editing.editPosition(column.positionAt(mouse.x, mouse.y))
                }
                onPressed: mouse => {
                    textEdit.forceActiveFocus()
                    textEdit.cursorPosition = editAt(mouse)
                }
                onPositionChanged: mouse => {
                    if (pressed)
                        textEdit.moveCursorSelection(editAt(mouse), TextEdit.SelectCharacters)
                }
            }
        }
    }

    // Touche prise par l'éditeur pendant l'édition : Échap, Ctrl+B, Ctrl+I, Ctrl+U (sans autre modificateur).
    function ownedKey(event) {
        if (event.key === Qt.Key_Escape)
            return !root.listOpen
        return event.modifiers === Qt.ControlModifier
                && (event.key === Qt.Key_B || event.key === Qt.Key_I || event.key === Qt.Key_U)
    }

    // De haut en bas : sélection et curseur du TextEdit d'entrée, placés dans la colonne montrée.
    function syncColumn() {
        if (!shown || !editing.vertical)
            return
        const start = textEdit.selectionStart
        const end = textEdit.selectionEnd
        if (start === end)
            column.deselect()
        else
            column.select(editing.columnPosition(start, true), editing.columnPosition(end, false))
        caret.at = column.positionToRectangle(editing.columnPosition(textEdit.cursorPosition, false))
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
        visible: root.shown && !root.editing.vertical // F-250 : la colonne d'un texte de haut en bas ne s'étend pas
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
