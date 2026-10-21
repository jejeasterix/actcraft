import QtQuick
import ActCraft.Ui

// Curseur par outil (F-201) : flèche pour la sélection, croix pour le stylo ; pour le marqueur et la gomme, le
// curseur système est masqué (Qt.BlankCursor) et le cercle de la largeur de l'outil, au zoom, est l'indicateur de
// largeur de PageArea (F-138), qui suit déjà tools.width : un seul cercle, jamais deux. PageArea lit cursorShape.
Item {
    id: cursor
    required property Tools tools
    readonly property bool circled: cursor.tools.currentTool === "highlighter" || cursor.tools.currentTool === "eraser"
    property int cursorShape: cursor.tools.currentTool === "select"
                              ? Qt.ArrowCursor
                              : cursor.circled ? Qt.BlankCursor : Qt.CrossCursor
    enabled: false
}