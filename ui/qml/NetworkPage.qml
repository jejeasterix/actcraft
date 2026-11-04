import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Section Réseau des paramètres (F-930, Windows) : mandataire du poste. L'hôte, le port et le compte vont aux
// préférences du poste ; le mot de passe va au trousseau par setPassword() et la page ne le relit jamais.
Item {
    id: root
    objectName: "networkPage"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    implicitWidth: 460
    implicitHeight: column.implicitHeight

    NetworkSettings { id: network }

    ColumnLayout {
        id: column
        anchors.fill: parent
        spacing: 6

        Label { text: qsTr("Mandataire réseau"); font.bold: true }
        Label {
            objectName: "networkUnavailableLabel"
            visible: !network.available
            text: qsTr("Réglage des postes Windows : ce poste n'a pas de trousseau.")
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        GridLayout {
            columns: 2
            enabled: network.available
            Layout.fillWidth: true

            Label { text: qsTr("Hôte") }
            TextField {
                objectName: "proxyHostField"
                Layout.fillWidth: true
                placeholderText: qsTr("aucun mandataire")
                text: network.host
                onEditingFinished: network.host = text
            }
            Label { text: qsTr("Port") }
            SpinBox {
                objectName: "proxyPortBox"
                from: 1; to: 65535; editable: true
                value: network.port
                onValueModified: network.port = value
            }
            Label { text: qsTr("Compte") }
            TextField {
                objectName: "proxyUserField"
                Layout.fillWidth: true
                text: network.user
                onEditingFinished: network.user = text
            }
            Label { text: qsTr("Mot de passe") }
            TextField {
                id: passwordField
                objectName: "proxyPasswordField"
                Layout.fillWidth: true
                echoMode: TextInput.Password
                placeholderText: qsTr("gardé dans le trousseau du poste")
                // Un champ quitté vide ne l'efface pas : seul « Aucun mandataire » efface le mot de passe.
                onEditingFinished: {
                    if (passwordField.text.length > 0) {
                        network.setPassword(passwordField.text)
                        passwordField.text = ""
                    }
                }
            }
        }
        RowLayout {
            enabled: network.available
            Button {
                objectName: "proxyApplyButton"
                text: qsTr("Appliquer")
                onClicked: network.apply()
            }
            Button {
                objectName: "proxyClearButton"
                text: qsTr("Aucun mandataire")
                onClicked: network.clear()
            }
        }
    }
}
