import QtQuick
import ActCraft.Ui

// Curseur par outil (F-201) : flèche pour la sélection, croix pour le stylo ; pour le marqueur et la gomme, le
// curseur système est masqué (Qt.BlankCursor) et le cercle de la largeur de l'outil, au zoom, est l'indicateur de
// largeur de PageArea (F-138), qui suit déjà tools.width : un seul cercle, jamais deux. PageArea lit cursorShape.
// Outil des formes (S7.1) : flèche, suivie de la miniature de la forme courante (notre propre contour,
// appTools.shapeOutline), un peu à droite et au-dessous du pointeur.
Item {
    id: cursor
    required property Tools tools
    // Vue de la page : seule source de position à bouton enfoncé (le HoverHandler ne bouge pas pendant un glisser).
    property var view: null
    // Position du pointeur dans le repère de ce curseur : la plus récente des deux sources.
    property point pointerAt: Qt.point(0, 0)
    readonly property bool circled: cursor.tools.currentTool === "highlighter" || cursor.tools.currentTool === "eraser"
    readonly property bool shaped: cursor.tools.currentTool === "shape"
    property int cursorShape: cursor.tools.currentTool === "select" || cursor.shaped
                              ? Qt.ArrowCursor
                              : cursor.circled ? Qt.BlankCursor : Qt.CrossCursor
    readonly property var outline: cursor.shaped ? cursor.tools.shapeOutline(cursor.tools.shapePreset) : []

    HoverHandler {
        id: pointer
        enabled: cursor.shaped
        onPointChanged: cursor.pointerAt = pointer.point.position
    }
    Connections {
        target: cursor.view
        enabled: cursor.view !== null
        function onPointerPositionChanged() {
            cursor.pointerAt = cursor.mapFromItem(cursor.view, cursor.view.pointerPosition)
        }
    }

    Canvas {
        id: thumbnail
        objectName: "shapeCursorThumbnail"
        visible: cursor.shaped && pointer.hovered
        x: cursor.pointerAt.x + 14
        y: cursor.pointerAt.y + 14
        width: 20
        height: 20
        readonly property var outline: cursor.outline
        onOutlineChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            for (const pass of [[Theme.texte, 3], [Theme.barre, 1.5]]) {
                ctx.strokeStyle = pass[0]
                ctx.lineWidth = pass[1]
                ctx.beginPath()
                outline.forEach((p, i) => {
                    if (i === 0)
                        ctx.moveTo(1 + p.x * 18, 1 + p.y * 18)
                    else
                        ctx.lineTo(1 + p.x * 18, 1 + p.y * 18)
                })
                ctx.stroke()
            }
        }
    }
}