import QtQuick
import QtQuick.Controls
import ActCraft.Ui

// Lecteur d'aide hors ligne (D6-10) : F1 (help.contents) ouvre la page de l'outil courant, sinon l'index.
// Aucun moteur web, aucun chargement réseau : le modèle ne suit que les pages embarquées.
Item {
    id: root
    objectName: "helpViewer"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    // Nom de la page montrée (« pen », « deux-utilisateurs », « index ») ; lu par les scénarios du banc (L11-T18).
    readonly property string currentPage: help.currentPage
    visible: false

    HelpViewerModel {
        id: help
        onExternalLink: link => Qt.openUrlExternally(link)
    }

    // `page` : nom de page demandé par l'appelant (L11-C12 : argument de help.contents), sinon l'outil courant.
    function open(page) {
        help.openFor(typeof page === "string" && page !== "" ? page : (root.tools ? root.tools.currentTool : ""))
        root.visible = true
        root.forceActiveFocus() // Échap atteint l'item, pas le raccourci de tool.select
    }

    function close() {
        root.visible = false
    }

    Rectangle {
        anchors.fill: parent
        color: "white"
        border.color: "#888888"

        ScrollView {
            anchors.fill: parent
            anchors.margins: 12
            TextArea {
                objectName: "helpText"
                readOnly: true
                wrapMode: TextEdit.Wrap
                textFormat: TextEdit.RichText
                text: help.html
                onLinkActivated: link => help.follow(link)
            }
        }
        Button {
            objectName: "helpClose"
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 6
            text: qsTr("Fermer")
            onClicked: root.close()
        }
    }

    // Échap est aussi le raccourci de tool.select : le lecteur ouvert le garde pour lui.
    Keys.onShortcutOverride: event => { event.accepted = event.key === Qt.Key_Escape }
    Keys.onEscapePressed: root.close()

    Component.onCompleted: {
        if (root.commands)
            root.commands.setHandler("help.contents", function (page) { root.open(page) })
    }
}
