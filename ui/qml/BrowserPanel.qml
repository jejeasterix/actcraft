import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Panneau des navigateurs (F-040 à F-042, S7.4) : rangée d'icônes (pages actif ; ressources, objets, notes,
// propriétés et actions grisés jusqu'au lot 6, sans vote), navigateur de pages dessous. Affiché ou caché par
// view.browsers (Ctrl+B), ancré à gauche ou à droite de la zone de page (side), largeur réglée en glissant son
// bord intérieur, de minWidth à maxWidth. Le panneau est dans la ligne de la page : il ne la recouvre pas, il
// n'est donc pas une couverture d'entrée de la vue.
Rectangle {
    id: root
    objectName: "browserPanel"
    required property PageThumbnails thumbnails
    required property Pages pages
    required property DocumentController documentController
    required property Commands commands
    property string side: "left" // « left » ou « right »
    readonly property int minWidth: 160
    readonly property int maxWidth: 480
    property int panelWidth: 220
    readonly property int iconSize: 30

    visible: false
    color: Theme.palette
    Layout.fillHeight: true
    Layout.preferredWidth: Math.max(root.minWidth, Math.min(root.maxWidth, root.panelWidth))

    // Place du panneau dans la ligne : avant ou après la zone de page (l'ordre des enfants fixe celui de la
    // ligne ; la mise en page est forcée ensuite).
    function dock() {
        const row = root.parent
        if (!row)
            return
        let area = null
        for (let i = 0; i < row.children.length; ++i) {
            if (row.children[i].objectName === "pageArea")
                area = row.children[i]
        }
        if (!area)
            return
        // Un simple stackBefore / stackAfter ne refait pas la ligne : l'enfant est retiré puis remis (la ligne se
        // reconstruit alors à sa prochaine mise en page, dans l'ordre fixé ensuite).
        root.parent = null
        root.parent = row
        if (root.side === "right")
            root.stackAfter(area)
        else
            root.stackBefore(area)
    }
    onSideChanged: root.dock()

    // État coché de « Navigateurs » (Ctrl+B, icône, menu) : celui du panneau. Lu par le banc.
    readonly property bool commandChecked: root.commands.revision >= 0 && root.commands.isChecked("view.browsers")
    function syncChecked() {
        root.commands.setChecked("view.browsers", root.visible)
    }
    onVisibleChanged: root.syncChecked()
    Component.onCompleted: root.syncChecked()

    // Les cinq icônes à nous (nav-*) et celle des pages (page-copier, déjà à nous).
    readonly property var navigators: [
        { id: "pages", label: qsTr("Pages"), icon: "page-copier", enabled: true },
        { id: "ressources", label: qsTr("Ressources"), icon: "nav-ressources", enabled: false },
        { id: "objets", label: qsTr("Objets"), icon: "nav-objets", enabled: false },
        { id: "notes", label: qsTr("Notes"), icon: "nav-notes", enabled: false },
        { id: "proprietes", label: qsTr("Propriétés"), icon: "nav-proprietes", enabled: false },
        { id: "actions", label: qsTr("Actions"), icon: "nav-actions", enabled: false }
    ]

    Flow {
        id: iconRow
        objectName: "navigatorIcons"
        x: Theme.espace2
        y: Theme.espace2
        width: root.width - 2 * Theme.espace2
        spacing: 2
        Repeater {
            model: root.navigators
            delegate: ToolButton {
                id: navButton
                required property var modelData
                objectName: "navigator-" + modelData.id
                width: root.iconSize
                height: root.iconSize
                enabled: modelData.enabled
                checked: modelData.id === "pages"
                display: AbstractButton.IconOnly
                focusPolicy: Qt.NoFocus
                icon.source: "qrc:/qt/qml/ActCraft/Ui/icons/" + modelData.icon + ".svg"
                icon.width: Theme.tailleIcone
                icon.height: Theme.tailleIcone
                icon.color: !navButton.enabled ? Theme.texteDesactive
                            : navButton.checked ? Theme.texteSurActive : Theme.texte
                Accessible.name: modelData.label
                ToolTip.visible: hovered
                ToolTip.delay: 600
                ToolTip.text: modelData.label
                background: Rectangle {
                    radius: Theme.rayon
                    color: navButton.checked ? Theme.surfaceActive
                           : navButton.hovered ? Theme.surfaceSurvol : Theme.surface
                }
            }
        }
    }

    PageNavigator {
        objectName: "pageNavigator"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: iconRow.bottom
        anchors.bottom: parent.bottom
        anchors.topMargin: Theme.espace2
        thumbnails: root.thumbnails
        pages: root.pages
        documentController: root.documentController
        commands: root.commands
    }

    // Bord intérieur : glissé, il règle la largeur (160 à 480).
    Rectangle {
        id: grip
        objectName: "browserGrip"
        width: 6
        height: parent.height
        x: root.side === "left" ? parent.width - width : 0
        color: gripArea.pressed ? Theme.surfaceActive : Theme.separateur
        MouseArea {
            id: gripArea
            anchors.fill: parent
            cursorShape: Qt.SizeHorCursor
            property real startX: 0
            property int startWidth: 0
            onPressed: mouse => {
                startX = mapToItem(null, mouse.x, 0).x
                startWidth = root.panelWidth
            }
            onPositionChanged: mouse => {
                if (!pressed)
                    return
                const dx = mapToItem(null, mouse.x, 0).x - startX
                root.panelWidth = Math.max(root.minWidth, Math.min(root.maxWidth,
                    startWidth + (root.side === "left" ? dx : -dx)))
            }
        }
    }
}
