import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Résumé à l'enregistrement (F-930, réglage document/summary-on-save) et vidage des documents récents.
Item {
    id: root
    objectName: "saveSummaryDialog"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    // Liste des documents récents (fileActions.recents) ; sinon le vidage passe par les préférences du poste.
    property var recents: null
    property var summary: ({ name: "", pages: 0, objects: 0, resources: 0, bytes: 0 })
    visible: false

    function show(summary) {
        root.summary = summary
        popup.open()
    }
    function sizeText(bytes) {
        if (bytes >= 1048576) return qsTr("%1 Mo").arg((bytes / 1048576).toFixed(1))
        if (bytes >= 1024) return qsTr("%1 Ko").arg(Math.round(bytes / 1024))
        return qsTr("%1 octets").arg(bytes)
    }

    Popup {
        id: popup
        objectName: "saveSummaryPopup"
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        padding: 16
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        contentItem: ColumnLayout {
            spacing: 8
            Label {
                objectName: "summaryName"
                text: qsTr("%1 est enregistré").arg(root.summary.name)
                font.bold: true
            }
            Label { objectName: "summaryPages"; text: qsTr("Pages : %1").arg(root.summary.pages) }
            Label { objectName: "summaryObjects"; text: qsTr("Objets : %1").arg(root.summary.objects) }
            Label { objectName: "summaryResources"; text: qsTr("Ressources : %1").arg(root.summary.resources) }
            Label { objectName: "summarySize"; text: qsTr("Taille du fichier : %1").arg(root.sizeText(root.summary.bytes)) }
            Button { text: qsTr("OK"); Layout.alignment: Qt.AlignRight; onClicked: popup.close() }
        }
    }

    Component.onCompleted: {
        root.commands.setHandler("file.recent-clear", function () {
            if (root.recents)
                root.recents.clear()
            else if (root.preferences)
                root.preferences.remove("workstation/recent-documents")
        })
    }
}
