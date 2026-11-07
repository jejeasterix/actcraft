import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Boîte du cadre web (insert.web-frame, F-302, S27) : une adresse ou du code HTML ; « OK » crée le cadre en une
// étape (WebFrameItems.insertFrame). Ouverte par WebFrameLayer.qml, qui lui donne son modèle des cadres. Une
// adresse saisie ici est la seule voie distante d'un document créé dans la session.
Item {
    id: root
    visible: false
    objectName: "webFrameDialog"
    property var frames: null
    property alias address: addressField.text
    property alias code: codeField.text
    readonly property bool opened: dialog.opened
    // « OK » ne crée rien tant que l'adresse et le code sont vides (le modèle refuserait de toute façon).
    readonly property bool canInsert: address.trim().length > 0 || code.trim().length > 0

    function open(target) {
        frames = target
        addressField.text = ""
        codeField.text = ""
        dialog.open()
        addressField.forceActiveFocus()
    }

    Dialog {
        id: dialog
        objectName: "webFrameDialogBox"
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        width: 540
        title: qsTr("Cadre web")
        standardButtons: Dialog.Ok | Dialog.Cancel
        Component.onCompleted: standardButton(Dialog.Ok).enabled = Qt.binding(function () { return root.canInsert })
        onAccepted: {
            if (root.frames && root.canInsert)
                root.frames.insertFrame(codeField.text, addressField.text)
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 8
            Label {
                text: qsTr("Adresse de la page")
            }
            TextField {
                id: addressField
                objectName: "webFrameAddress"
                Layout.fillWidth: true
                placeholderText: "https://"
                inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoAutoUppercase
                selectByMouse: true
            }
            Label {
                text: qsTr("Ou code HTML")
            }
            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 160
                TextArea {
                    id: codeField
                    objectName: "webFrameCode"
                    wrapMode: TextArea.Wrap
                    textFormat: TextEdit.PlainText
                    selectByMouse: true
                    placeholderText: "<p>…</p>"
                }
            }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                opacity: 0.7
                text: qsTr("JavaScript reste coupé. Dans un document reçu, le contenu distant reste bloqué jusqu'à ce que vous débloquiez sa page.")
            }
        }
    }
}
