import QtQuick
import QtQuick.Shapes
import ActCraft.Ui

// Curseur par outil (F-201) : flèche pour la sélection, croix pour le stylo ; pour le marqueur et la gomme, le
// curseur système est masqué (Qt.BlankCursor) et le cercle de la largeur de l'outil, au zoom, est l'indicateur de
// largeur de PageArea (F-138), qui suit déjà tools.width : un seul cercle, jamais deux. PageArea lit cursorShape.
// Outil des formes (S7.1) : flèche, suivie de la miniature de la forme courante (notre propre contour,
// appTools.shapeOutline), un peu à droite et au-dessous du pointeur.
Item {
    id: cursor
    required property Tools tools
    readonly property bool circled: cursor.tools.currentTool === "highlighter" || cursor.tools.currentTool === "eraser"
    readonly property bool shaped: cursor.tools.currentTool === "shape"
    property int cursorShape: cursor.tools.currentTool === "select" || cursor.shaped
                              ? Qt.ArrowCursor
                              : cursor.circled ? Qt.BlankCursor : Qt.CrossCursor
    readonly property var outline: cursor.shaped ? cursor.tools.shapeOutline(cursor.tools.shapePreset) : []

    HoverHandler {
        id: pointer
        enabled: cursor.shaped
    }

    Shape {
        objectName: "shapeCursorThumbnail"
        visible: cursor.shaped && pointer.hovered
        x: pointer.point.position.x + 14
        y: pointer.point.position.y + 14
        width: 20
        height: 20
        ShapePath {
            strokeColor: Theme.texte
            strokeWidth: 3
            fillColor: "transparent"
            PathPolyline {
                path: cursor.outline.map(p => Qt.point(1 + p.x * 18, 1 + p.y * 18))
            }
        }
        ShapePath {
            strokeColor: Theme.barre
            strokeWidth: 1.5
            fillColor: "transparent"
            PathPolyline {
                path: cursor.outline.map(p => Qt.point(1 + p.x * 18, 1 + p.y * 18))
            }
        }
    }
}
