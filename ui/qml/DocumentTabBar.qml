import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Barre d'onglets des paperboards (F-010 à F-012, D14). Visible dès deux documents ouverts, sauf position
// « hidden » ; en haut sous la ligne supérieure, ou en bas (Main.qml en règle les ancres d'après `tabs.position`).
// Le nom de l'onglet affiché est en gras ; un astérisque suit le titre d'un document modifié (le modèle ne le
// porte pas). Le titre vient d'un fichier : texte brut, jamais interprété comme du balisage. Un onglet n'est
// activé ni fermé pendant une lecture ou une écriture : le modèle refuse sans erreur. Déposer des pages sur un
// onglet inactif les y copie (F-011, voir PageNavigator). Ctrl+Alt+N ouvre un onglet neuf.
Item {
    id: root
    objectName: "documentTabBar"
    required property DocumentTabs tabs
    required property Commands commands
    signal popupOpened()
    signal popupClosed()

    readonly property int tabHeight: 40
    readonly property bool shown: root.tabs.count >= 2 && root.tabs.position !== "hidden"
    // Fermeture en attente d'un enregistrement réussi (onglet modifié, « Enregistrer » choisi).
    property bool closeAfterSave: false

    visible: root.shown
    height: root.shown ? root.tabHeight : 0

    function askClose(index) {
        // L'enregistrement vise le document affiché : l'onglet visé passe d'abord au premier plan.
        if (index !== root.tabs.currentIndex)
            root.tabs.activate(index)
        if (index !== root.tabs.currentIndex)
            return // lecture ou écriture en cours : rien ne bouge
        saveChanges.documentTitle = root.tabs.document.title
        saveChanges.open()
    }

    Shortcut {
        sequence: "Ctrl+Alt+N"
        context: Qt.ApplicationShortcut
        onActivated: root.tabs.newTab()
    }

    Connections {
        target: root.tabs
        function onSaveRequested(index) { root.askClose(index) }
        function onCurrentIndexChanged() { root.closeAfterSave = false }
    }
    Connections {
        target: root.tabs.document
        function onSaveFinished(ok, message) {
            if (!root.closeAfterSave)
                return
            root.closeAfterSave = false
            if (ok && !root.tabs.document.modified)
                root.tabs.closeWithoutSaving(root.tabs.currentIndex)
        }
    }

    SaveChangesDialog {
        id: saveChanges
        objectName: "tabSaveChangesDialog"
        onOpened: root.popupOpened()
        onClosed: root.popupClosed()
        onSaveRequested: {
            root.closeAfterSave = true
            root.commands.trigger("file.save")
        }
        onDiscardRequested: root.tabs.closeWithoutSaving(root.tabs.currentIndex)
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.barre
    }
    // Filet du côté de la page.
    Rectangle {
        height: 1
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: root.tabs.position === "bottom" ? parent.top : undefined
        anchors.bottom: root.tabs.position === "bottom" ? undefined : parent.bottom
        color: Theme.separateur
    }

    ListView {
        id: strip
        objectName: "documentTabStrip"
        anchors.left: parent.left
        anchors.right: addButton.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        orientation: ListView.Horizontal
        spacing: 2
        clip: true
        interactive: contentWidth > width
        boundsBehavior: Flickable.StopAtBounds
        model: root.tabs

        delegate: Rectangle {
            id: tab
            required property int index
            required property string title
            required property bool modified
            required property bool current
            objectName: "documentTab" + tab.index
            width: Math.min(220, Math.max(120, strip.width / Math.max(1, strip.count) - strip.spacing))
            height: strip.height
            color: dropZone.containsDrag ? Theme.surfaceActive
                   : tab.current ? Theme.palette
                   : select.containsMouse ? Theme.surfaceSurvol : Theme.surface
            Accessible.role: Accessible.PageTab
            Accessible.name: label.text
            // Filet d'accent de l'onglet affiché.
            Rectangle {
                visible: tab.current
                height: 3
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: root.tabs.position === "bottom" ? undefined : parent.top
                anchors.bottom: root.tabs.position === "bottom" ? parent.bottom : undefined
                color: Theme.surfaceActive
            }
            MouseArea {
                id: select
                objectName: "documentTabSelect" + tab.index
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.MiddleButton
                onClicked: mouse => {
                    if (mouse.button === Qt.MiddleButton)
                        root.tabs.requestClose(tab.index)
                    else
                        root.tabs.activate(tab.index)
                }
            }
            Label {
                id: label
                objectName: "documentTabLabel" + tab.index
                anchors.left: parent.left
                anchors.leftMargin: Theme.espace2
                anchors.right: closeArea.left
                anchors.verticalCenter: parent.verticalCenter
                text: (tab.title.length > 0 ? tab.title : qsTr("Sans titre")) + (tab.modified ? "*" : "")
                textFormat: Text.PlainText
                elide: Text.ElideRight
                font.pixelSize: Theme.taillePolice
                font.bold: tab.current
                color: dropZone.containsDrag ? Theme.texteSurActive : Theme.texte
            }
            Item {
                id: closeArea
                objectName: "documentTabClose" + tab.index
                width: root.tabHeight - 4
                height: root.tabHeight - 4
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Fermer l'onglet")
                Label {
                    anchors.centerIn: parent
                    text: "×"
                    font.pixelSize: Theme.taillePoliceTitre
                    color: dropZone.containsDrag ? Theme.texteSurActive : Theme.texte
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: root.tabs.requestClose(tab.index)
                }
            }
            // Pages déposées depuis le navigateur : copiées avant la première page de cet onglet. L'onglet
            // affiché ne reçoit rien (le navigateur y déplace les pages).
            DropArea {
                id: dropZone
                objectName: "documentTabDrop" + tab.index
                anchors.fill: parent
                enabled: !tab.current
                keys: ["actcraft-pages"]
                onDropped: drop => {
                    let pages = []
                    try {
                        pages = JSON.parse(drop.getDataAsString("actcraft-pages"))
                    } catch (e) {
                        pages = []
                    }
                    if (pages.length === 0)
                        return
                    root.tabs.copyPagesToTab(pages, tab.index)
                    drop.acceptProposedAction()
                }
            }
        }
    }

    Item {
        id: addButton
        objectName: "documentTabAdd"
        width: root.tabHeight
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        Accessible.role: Accessible.Button
        Accessible.name: qsTr("Nouvel onglet")
        Label {
            anchors.centerIn: parent
            text: "+"
            font.pixelSize: Theme.taillePoliceTitre
            color: Theme.texte
        }
        MouseArea {
            anchors.fill: parent
            onClicked: root.tabs.newTab()
        }
    }
}
