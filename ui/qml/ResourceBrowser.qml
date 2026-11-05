// Navigateur de ressources (F-810 à F-813, S9.6) : moitié haute (racines, chemin, recherche, arbre des
// dossiers), moitié basse (compteur, vignettes ou liste, tampon). La bibliothèque (ResourceLibrary) fait tout
// le travail ; ce fichier montre ses lignes, traduit clic, double-clic et glisser, et confirme les suppressions.
// Tampon (D13) : tant qu'il est actif, une couverture d'entrée recouvre la page (A7) ; chaque clic y pose une
// copie de l'élément choisi. Le tampon ne trace jamais de trait.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import ActCraft.Ui

Item {
    id: root
    objectName: "resourceBrowser"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    visible: false

    // Élément choisi (chemin) : cible du menu, du double-clic et du tampon. Dossier choisi : cible du menu des dossiers.
    property string selectedPath: ""
    property string selectedFolder: ""
    property bool stamping: false
    readonly property int rowHeight: 28

    ResourceLibrary {
        id: appResources
        objectName: "appResources"
        document: root.documentController
        profile: root.profile
        commands: root.commands
        tools: root.tools
        contactActive: root.view ? root.view.contactActive : false
        onTrashFailed: path => {
            permanentDialog.target = path
            permanentDialog.open()
        }
        onFolderChanged: root.selectedFolder = ""
    }

    function pageCenter() {
        const size = root.documentController ? root.documentController.pageSize : Qt.size(1024, 768)
        return Qt.point(size.width / 2, size.height / 2)
    }
    function insertAt(path, pagePoint) {
        if (path === "")
            return false
        root.selectedPath = path
        return appResources.insert(path, pagePoint)
    }
    function setStamping(on) {
        if (on && root.view && stampCover.parent !== root.view.parent)
            stampCover.parent = root.view.parent
        root.stamping = on
        if (root.commands)
            root.commands.setChecked("resources.stamp", on)
    }
    // Un clic ou un relâché en coordonnées de la scène : le point tombe-t-il sur ce navigateur (dossiers ou moitié basse) ?
    function sceneOverDropZone(scene) {
        const inTree = treeList.contains(treeList.mapFromItem(null, scene.x, scene.y))
        const inItems = itemsArea.contains(itemsArea.mapFromItem(null, scene.x, scene.y))
        return root.visible && (inTree || inItems)
    }

    Component.onCompleted: {
        root.commands.setHandler("resources.mine", function () { appResources.setRoot("mine") })
        root.commands.setHandler("resources.shared", function () { appResources.setRoot("shared") })
        root.commands.setHandler("resources.other", function (url) {
            if (url !== undefined && url !== null && String(url) !== "")
                appResources.setOtherFolder(url)
            else
                otherDialog.open()
        })
        root.commands.setHandler("resources.insert", function (path) {
            root.insertAt(path !== undefined && path !== null && String(path) !== "" ? String(path) : root.selectedPath,
                          root.pageCenter())
        })
        root.commands.setHandler("resources.add", function () {
            const added = appResources.addSelection()
            if (added !== "")
                root.selectedPath = added
        })
        root.commands.setHandler("resources.stamp", function () { root.setStamping(!root.stamping) })
        root.commands.setHandler("resources.new-folder", function (name) {
            if (name !== undefined && name !== null && String(name) !== "")
                appResources.createFolder(String(name))
            else
                nameDialog.openFor("new", "", "")
        })
        root.commands.setHandler("resources.rename", function (name) {
            const target = root.selectedPath !== "" ? root.selectedPath : root.selectedFolder
            if (target === "")
                return
            if (name !== undefined && name !== null && String(name) !== "")
                appResources.rename(target, String(name))
            else
                nameDialog.openFor("rename", target, "")
        })
        root.commands.setHandler("resources.delete", function (path) {
            const target = path !== undefined && path !== null && String(path) !== "" ? String(path)
                           : root.selectedPath !== "" ? root.selectedPath : root.selectedFolder
            if (target !== "") {
                confirmDialog.target = target
                confirmDialog.open()
            }
        })
        if (root.view)
            root.view.addInputCover(stampCover)
    }
    Component.onDestruction: if (root.view) root.view.removeInputCover(stampCover)

    // Glisser la sélection de la page jusqu'à ce navigateur : au relâché sur un dossier ou la moitié basse, ajout.
    Connections {
        target: root.view
        function onContactActiveChanged() {
            if (!root.view || root.view.contactActive || root.stamping || !root.visible)
                return
            if (!root.tools || root.tools.currentTool !== "select" || root.tools.selectionCount < 1)
                return
            const scene = root.view.mapToItem(null, root.view.pointerPosition.x, root.view.pointerPosition.y)
            if (root.sceneOverDropZone(scene))
                root.commands.trigger("resources.add")
        }
    }

    // Couverture d'entrée du tampon : sœur de la vue, au-dessus d'elle.
    Item {
        id: stampCover
        objectName: "stampCover"
        x: root.view ? root.view.x : 0
        y: root.view ? root.view.y : 0
        width: root.view ? root.view.width : 0
        height: root.view ? root.view.height : 0
        z: 1000
        visible: root.stamping
        enabled: root.stamping
        TapHandler {
            acceptedButtons: Qt.LeftButton
            onTapped: (point, button) => {
                const scene = stampCover.mapToItem(null, point.position.x, point.position.y)
                root.insertAt(root.selectedPath, root.view.sceneToPage(scene))
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.surface
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.espace2
        spacing: Theme.espace1

        // Moitié haute
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.espace1
            Repeater {
                model: [
                    { id: "mine", label: qsTr("Mes ressources"), command: "resources.mine" },
                    { id: "shared", label: qsTr("Ressources partagées"), command: "resources.shared" },
                    { id: "other", label: qsTr("Autre dossier…"), command: "resources.other" }
                ]
                delegate: Button {
                    required property var modelData
                    objectName: "root-" + modelData.id
                    Layout.fillWidth: true
                    text: modelData.label
                    checkable: true
                    checked: appResources.root === modelData.id
                    focusPolicy: Qt.NoFocus
                    font.pixelSize: Theme.taillePolice - 2
                    onClicked: root.commands.trigger(modelData.command)
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Label {
                objectName: "folderPath"
                Layout.fillWidth: true
                text: appResources.folder
                color: Theme.texteDiscret
                font.pixelSize: Theme.taillePolice - 2
                elide: Text.ElideMiddle
            }
            ToolButton {
                objectName: "folderMenuButton"
                text: "…"
                focusPolicy: Qt.NoFocus
                onClicked: folderMenu.popup()
            }
        }
        TextField {
            id: searchField
            objectName: "resourceSearch"
            Layout.fillWidth: true
            placeholderText: qsTr("Rechercher…")
            onTextEdited: text === "" ? appResources.cancelSearch() : appResources.search(text, true, true)
            onAccepted: text === "" ? appResources.cancelSearch() : appResources.search(text, true, true)
        }
        ListView {
            id: treeList
            objectName: "folderTree"
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: appResources.folders
            delegate: Item {
                id: folderRow
                required property var modelData
                width: treeList.width
                height: root.rowHeight
                readonly property bool current: modelData.path === appResources.folder
                Rectangle {
                    anchors.fill: parent
                    color: folderRow.current ? Theme.surfaceActive : "transparent"
                }
                Label {
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.espace1 + folderRow.modelData.depth * 16
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: folderRow.modelData.name
                    color: folderRow.current ? Theme.texteSurActive : Theme.texte
                    font.pixelSize: Theme.taillePolice
                    elide: Text.ElideRight
                }
                TapHandler {
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onTapped: (point, button) => {
                        root.selectedFolder = folderRow.modelData.path
                        appResources.openFolder(folderRow.modelData.path)
                        root.selectedFolder = folderRow.modelData.path
                        if (button === Qt.RightButton) {
                            root.selectedPath = ""
                            folderMenu.popup()
                        }
                    }
                }
            }
        }

        // Moitié basse
        RowLayout {
            Layout.fillWidth: true
            Label {
                objectName: "itemCountLabel"
                Layout.fillWidth: true
                text: qsTr("%n élément(s)", "", appResources.itemCount)
                color: Theme.texte
                font.pixelSize: Theme.taillePolice
            }
            ToolButton {
                objectName: "gridToggle"
                text: appResources.grid ? qsTr("Liste") : qsTr("Grille")
                focusPolicy: Qt.NoFocus
                onClicked: appResources.grid = !appResources.grid
            }
            ToolButton {
                objectName: "stampButton"
                text: qsTr("Tampon")
                checkable: true
                checked: root.stamping
                enabled: root.selectedPath !== ""
                focusPolicy: Qt.NoFocus
                onClicked: root.commands.trigger("resources.stamp")
            }
        }
        Item {
            id: itemsArea
            objectName: "resourceItems"
            Layout.fillWidth: true
            Layout.fillHeight: true

            GridView {
                id: grid
                anchors.fill: parent
                clip: true
                visible: appResources.grid
                boundsBehavior: Flickable.StopAtBounds
                cellWidth: appResources.thumbnailWidth + Theme.espace2
                cellHeight: appResources.thumbnailWidth + 24
                model: appResources
                delegate: ResourceCell { cellWidth: grid.cellWidth; cellHeight: grid.cellHeight }
            }
            ListView {
                id: list
                anchors.fill: parent
                clip: true
                visible: !appResources.grid
                boundsBehavior: Flickable.StopAtBounds
                model: appResources
                delegate: ResourceCell { cellWidth: list.width; cellHeight: root.rowHeight + 4; listMode: true }
            }
        }
    }

    // Une cellule (vignette ou ligne) : clic choisit, double-clic insère au centre, glisser sur la page insère
    // au point de dépôt, clic droit ouvre le menu des éléments.
    component ResourceCell: Item {
        id: cell
        required property string name
        required property string path
        required property string kind
        required property url thumbnail
        required property int index
        property real cellWidth: 100
        property real cellHeight: 100
        property bool listMode: false
        width: cellWidth
        height: cellHeight
        readonly property bool current: cell.path === root.selectedPath

        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            color: cell.current ? Theme.surfaceActive : area.containsMouse ? Theme.surfaceSurvol : "transparent"
        }
        Image {
            id: thumb
            x: Theme.espace1
            y: Theme.espace1
            width: cell.listMode ? root.rowHeight - 4 : cell.cellWidth - 2 * Theme.espace1
            height: cell.listMode ? root.rowHeight - 4 : cell.cellHeight - 24
            source: cell.thumbnail
            fillMode: Image.PreserveAspectFit
            asynchronous: true
        }
        Label {
            anchors.left: cell.listMode ? thumb.right : parent.left
            anchors.leftMargin: Theme.espace1
            anchors.right: parent.right
            anchors.top: cell.listMode ? undefined : thumb.bottom
            anchors.verticalCenter: cell.listMode ? parent.verticalCenter : undefined
            text: cell.name
            color: cell.current ? Theme.texteSurActive : Theme.texte
            font.pixelSize: Theme.taillePolice - 2
            elide: Text.ElideRight
        }
        Accessible.role: Accessible.ListItem
        Accessible.name: cell.name

        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            property bool dragged: false
            property point pressAt
            onPressed: mouse => {
                dragged = false
                pressAt = Qt.point(mouse.x, mouse.y)
                root.selectedPath = cell.path
            }
            onPositionChanged: mouse => {
                if (pressed && Math.abs(mouse.x - pressAt.x) + Math.abs(mouse.y - pressAt.y) > 8)
                    dragged = true
            }
            onReleased: mouse => {
                if (!dragged || !root.view || mouse.button !== Qt.LeftButton)
                    return
                const scene = area.mapToItem(null, mouse.x, mouse.y)
                if (root.view.contains(root.view.mapFromItem(null, scene.x, scene.y)))
                    root.insertAt(cell.path, root.view.sceneToPage(scene))
            }
            onDoubleClicked: root.insertAt(cell.path, root.pageCenter())
            onClicked: mouse => {
                if (mouse.button === Qt.RightButton) {
                    root.selectedPath = cell.path
                    itemMenu.popup()
                }
            }
        }
    }

    Menu {
        id: folderMenu
        objectName: "folderMenu"
        MenuItem {
            text: qsTr("Nouveau dossier")
            onTriggered: root.commands.trigger("resources.new-folder")
        }
        MenuItem {
            text: qsTr("Renommer…")
            enabled: root.selectedFolder !== ""
            onTriggered: nameDialog.openFor("rename", root.selectedFolder, "")
        }
        MenuItem {
            text: qsTr("Supprimer")
            enabled: root.selectedFolder !== ""
            onTriggered: {
                confirmDialog.target = root.selectedFolder
                confirmDialog.open()
            }
        }
    }
    Menu {
        id: itemMenu
        objectName: "itemMenu"
        MenuItem {
            text: qsTr("Insérer dans le paperboard")
            onTriggered: root.commands.trigger("resources.insert", root.selectedPath)
        }
        MenuItem {
            text: qsTr("Renommer…")
            onTriggered: nameDialog.openFor("rename", root.selectedPath, "")
        }
        MenuItem {
            text: qsTr("Retirer de la bibliothèque")
            onTriggered: root.commands.trigger("resources.delete", root.selectedPath)
        }
    }

    FolderDialog {
        id: otherDialog
        objectName: "otherFolderDialog"
        title: qsTr("Choisir un dossier de ressources")
        onAccepted: appResources.setOtherFolder(selectedFolder)
    }

    // Nom d'un nouveau dossier ou nouveau nom d'un élément.
    Dialog {
        id: nameDialog
        objectName: "nameDialog"
        property string mode: "new"
        property string target: ""
        title: mode === "new" ? qsTr("Nouveau dossier") : qsTr("Renommer")
        modal: true
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Ok | Dialog.Cancel
        function openFor(m, t, initial) {
            mode = m
            target = t
            nameField.text = initial
            open()
            nameField.forceActiveFocus()
        }
        contentItem: TextField {
            id: nameField
            objectName: "nameField"
            onAccepted: nameDialog.accept()
        }
        onAccepted: {
            if (nameField.text === "")
                return
            if (mode === "new")
                appResources.createFolder(nameField.text)
            else
                appResources.rename(target, nameField.text)
        }
    }

    Dialog {
        id: confirmDialog
        objectName: "confirmDeleteDialog"
        property string target: ""
        title: qsTr("Retirer de la bibliothèque")
        modal: true
        width: 360
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Yes | Dialog.No
        contentItem: Label {
            text: qsTr("Retirer cet élément de la bibliothèque ?")
            color: Theme.texte
            wrapMode: Text.Wrap
        }
        onAccepted: {
            if (target === root.selectedPath)
                root.selectedPath = ""
            appResources.remove(target)
        }
    }
    // La corbeille a échoué : suppression définitive proposée, après confirmation.
    Dialog {
        id: permanentDialog
        objectName: "permanentDeleteDialog"
        property string target: ""
        title: qsTr("Suppression définitive")
        modal: true
        width: 360
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Yes | Dialog.No
        contentItem: Label {
            text: qsTr("La corbeille n'est pas disponible. Supprimer définitivement ?")
            color: Theme.texte
            wrapMode: Text.Wrap
        }
        onAccepted: appResources.remove(target, true)
    }
}
