import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Copieur de couleur (F-135) : « color.pick » capture la fenêtre une seule fois, puis une loupe de 7 × 7 pixels
// suit le pointeur ; un clic prend la couleur visée (couleur du stylo + palette), Échap annule. Tant qu'il est
// ouvert, le copieur couvre la fenêtre : aucun trait n'est tracé par son clic (A7).
Item {
    id: root
    objectName: "colorLoupe"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null

    readonly property int cells: 7
    readonly property int cellSize: 10
    property bool active: false
    property real pointerX: 0
    property real pointerY: 0

    anchors.fill: parent
    z: 1000
    visible: active
    focus: active

    ColorPalette {
        id: pal
        profile: root.profile
        baseColors: root.commands ? root.commands.colors : []
        onSnapshotRequested: grabFrom(root)
    }

    function close() { active = false }

    function open() {
        active = false // la capture ne montre pas la loupe
        pal.beginPick()
        active = true
        forceActiveFocus()
    }

    function choose(x, y) {
        const color = pal.colorAt(Math.round(x), Math.round(y))
        close()
        if (color)
            root.commands.trigger("color.add", color)
    }

    Keys.onEscapePressed: close()

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.AllButtons
        cursorShape: Qt.CrossCursor
        onPositionChanged: function (mouse) {
            root.pointerX = mouse.x
            root.pointerY = mouse.y
        }
        onClicked: function (mouse) { root.choose(mouse.x, mouse.y) }
    }

    Rectangle {
        id: loupe
        objectName: "loupe"
        readonly property int side: root.cells * root.cellSize
        width: side + 4
        height: side + 24
        x: Math.min(root.pointerX + 20, root.width - width)
        y: Math.min(root.pointerY + 20, root.height - height)
        color: Theme.palette
        border.width: 2
        border.color: Theme.cadreSelection
        radius: 3

        Grid {
            x: 2
            y: 2
            columns: root.cells
            Repeater {
                model: root.cells * root.cells
                delegate: Rectangle {
                    required property int index
                    readonly property int dx: index % root.cells - 3
                    readonly property int dy: Math.floor(index / root.cells) - 3
                    width: root.cellSize
                    height: root.cellSize
                    color: pal.colorAt(Math.round(root.pointerX) + dx, Math.round(root.pointerY) + dy) || "transparent"
                    border.width: dx === 0 && dy === 0 ? 2 : 0
                    border.color: "white"
                }
            }
        }
        Text {
            x: 2
            y: loupe.side + 4
            width: loupe.side
            horizontalAlignment: Text.AlignHCenter
            color: "white"
            font.pixelSize: 11
            text: pal.colorAt(Math.round(root.pointerX), Math.round(root.pointerY)).toUpperCase()
        }
    }

    Component.onCompleted: root.commands.setHandler("color.pick", function () { root.open() })
}
