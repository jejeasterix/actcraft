// Navigateur d'objet (F-820, F-821, S9.3) : une ligne par couche (en gras), puis les objets de la page
// affichée, du plus haut au plus bas. Le modèle (ObjectTreeModel) fait tout le travail sur le document ;
// ce fichier ne fait que montrer les lignes et traduire clic, Ctrl+clic et glisser vertical.
import QtQuick
import QtQuick.Controls
import ActCraft.Ui

Item {
    id: root
    objectName: "objectBrowser"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null

    readonly property int rowHeight: 28
    readonly property int indent: 16
    readonly property int iconSize: 18
    // Glisser en cours : ligne tenue, et ligne devant laquelle elle tomberait (-1 : aucun).
    property int dragFrom: -1
    property int dropBefore: -1

    // rowCount n'est pas une propriété du modèle C++ : le banc lit les propriétés, on la lui donne ici.
    component TreeModel: ObjectTreeModel {
        property int rowCount: 0
    }
    TreeModel {
        id: appObjectTree
        objectName: "appObjectTree"
        rowCount: list.count
        document: root.documentController
        tools: root.tools
        commands: root.commands
        contactActive: root.view ? root.view.contactActive : false
    }

    function kindIcon(kind) {
        switch (kind) {
        case "stroke": return "stylo"
        case "text": return "texte"
        case "image": return "image"
        case "media": return "media"
        case "group": return "grouper"
        case "connector": return "connecteur"
        case "equation": return "calculatrice"
        default: return "forme"
        }
    }

    component RowIcon: ToolButton {
        property string iconName: ""
        property bool lit: false
        property color tint: Theme.texte
        width: root.rowHeight - 4
        height: root.rowHeight - 4
        padding: 0
        display: AbstractButton.IconOnly
        focusPolicy: Qt.NoFocus
        icon.source: "qrc:/qt/qml/ActCraft/Ui/icons/" + iconName + ".svg"
        icon.width: root.iconSize
        icon.height: root.iconSize
        icon.color: tint
        opacity: lit ? 1.0 : 0.3
        background: Item {}
    }

    ListView {
        id: list
        objectName: "objectList"
        anchors.fill: parent
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: appObjectTree

        delegate: Item {
            id: row
            objectName: "objRow" + index
            width: list.width
            height: root.rowHeight
            readonly property bool isLayer: model.depth === 0
            readonly property bool isSelected: model.selected
            readonly property color textColor: row.isSelected ? Theme.texteSurActive
                                               : model.hidden ? Theme.texteDesactive : Theme.texte
            opacity: root.dragFrom === index ? 0.5 : 1.0

            Rectangle {
                anchors.fill: parent
                color: row.isSelected ? Theme.surfaceActive
                       : area.containsMouse && !row.isLayer ? Theme.surfaceSurvol : "transparent"
            }

            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                preventStealing: true
                acceptedButtons: Qt.LeftButton
                property real pressY: 0
                property bool dragged: false
                onPressed: mouse => {
                    pressY = mouse.y
                    dragged = false
                }
                onPositionChanged: mouse => {
                    if (!pressed || row.isLayer)
                        return
                    if (!dragged && Math.abs(mouse.y - pressY) < 6)
                        return
                    dragged = true
                    root.dragFrom = index
                    const y = mapToItem(list.contentItem, mouse.x, mouse.y).y
                    root.dropBefore = Math.max(0, Math.min(list.count, Math.round(y / root.rowHeight)))
                }
                onReleased: {
                    const from = root.dragFrom
                    const before = root.dropBefore
                    root.dragFrom = -1
                    root.dropBefore = -1
                    if (dragged && from >= 0 && before >= 0)
                        appObjectTree.move(from, before)
                }
                onClicked: mouse => {
                    if (!dragged)
                        appObjectTree.select(index, mouse.modifiers)
                }
            }

            RowIcon {
                id: kindMark
                visible: !row.isLayer
                enabled: false
                x: Theme.espace2 + root.indent
                anchors.verticalCenter: parent.verticalCenter
                iconName: root.kindIcon(model.kind)
                lit: true
                tint: row.textColor
            }
            Label {
                anchors.left: row.isLayer ? parent.left : kindMark.right
                anchors.leftMargin: row.isLayer ? Theme.espace2 : Theme.espace1
                anchors.right: hideMark.visible ? hideMark.left : parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: model.name
                textFormat: Text.PlainText // nom venu du fichier ouvert : aucune balise interprétée, rien de chargé
                color: row.textColor
                font.pixelSize: Theme.taillePolice
                font.bold: row.isLayer
                elide: Text.ElideRight
            }
            RowIcon {
                id: hideMark
                objectName: "hideMark" + index
                visible: !row.isLayer
                anchors.right: lockMark.left
                anchors.verticalCenter: parent.verticalCenter
                iconName: "nav-objets-masque"
                lit: model.hidden
                tint: row.textColor
                Accessible.name: qsTr("Masquer")
                onClicked: appObjectTree.setHidden(index, !model.hidden)
            }
            RowIcon {
                id: lockMark
                objectName: "lockMark" + index
                visible: !row.isLayer
                anchors.right: parent.right
                anchors.rightMargin: Theme.espace1
                anchors.verticalCenter: parent.verticalCenter
                iconName: "nav-objets-verrou"
                lit: model.locked
                tint: row.textColor
                Accessible.name: qsTr("Verrouiller")
                onClicked: appObjectTree.setLocked(index, !model.locked)
            }
            Accessible.role: Accessible.ListItem
            Accessible.name: model.name
        }

        // Fantôme d'insertion : filet posé entre deux lignes pendant le glisser.
        Rectangle {
            objectName: "dropGhost"
            visible: root.dragFrom >= 0 && root.dropBefore >= 0
            x: 0
            y: root.dropBefore * root.rowHeight - 1
            width: list.width
            height: 2
            z: 10
            color: Theme.surfaceActive
        }
    }
}
