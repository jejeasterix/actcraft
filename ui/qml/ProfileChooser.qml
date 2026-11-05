import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Choix du profil (F-123) : un clic sur un profil l'active et ferme la fenêtre.
Item {
    id: root
    objectName: "profileChooser"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    readonly property bool opened: popup.opened
    visible: false

    Popup {
        id: popup
        objectName: "profileChooserPopup"
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        padding: 12
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        contentItem: ColumnLayout {
            spacing: 6
            Label { text: qsTr("Changer de profil"); font.bold: true }
            Repeater {
                model: root.profile ? root.profile.profiles : []
                delegate: Rectangle {
                    id: row
                    required property var modelData
                    objectName: "profileRow-" + modelData.id
                    Layout.fillWidth: true
                    Layout.preferredWidth: 280
                    implicitHeight: 36
                    radius: 4
                    color: modelData.id === root.profile.currentId ? "#cfe3ff" : (hover.hovered ? "#eef3fa" : "transparent")
                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 8
                        text: row.modelData.label
                        textFormat: Text.PlainText // libellé d'un profil importé : texte brut
                        font.bold: row.modelData.id === root.profile.currentId
                    }
                    HoverHandler { id: hover }
                    TapHandler {
                        onTapped: {
                            root.commands.trigger("profile.activate", row.modelData.id)
                            popup.close()
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        root.commands.setHandler("profile.choose", function () { popup.open() })
    }
}
