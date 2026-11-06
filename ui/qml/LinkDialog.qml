import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import ActCraft.Ui

// Création d'un lien vers un fichier ou un site (F-550, F-551) : cible, forme du lien (texte, icône, objet
// d'action, objet existant de la page) et, pour un fichier, son stockage (chemin gardé ou rangé dans le
// document). Le contrôleur crée le lien en une étape et refuse avec le motif de l'ouverture : la boîte reste
// ouverte et montre ce motif. Rien ne s'ouvre ici ; l'ouverture est celle du clic, après confirmation.
Dialog {
    id: dialog
    objectName: "linkDialog"
    property var commands: null
    property LinkDialogs controller: null
    property bool fileMode: false
    property string error: ""
    title: fileMode ? qsTr("Lien vers un fichier") : qsTr("Lien vers un site")
    modal: true
    width: 480
    anchors.centerIn: Overlay.overlay
    closePolicy: Popup.CloseOnEscape

    function begin(isFile, argument) {
        dialog.fileMode = isFile
        dialog.error = ""
        let text = ""
        if (argument !== undefined && argument !== null)
            text = String(argument)
        if (isFile && text.startsWith("file:"))
            text = decodeURIComponent(text.replace(/^file:\/\/(\/(?=[A-Za-z]:))?/, ""))
        targetField.text = text
        formBox.currentIndex = 0
        storeBox.checked = false
        argumentsField.text = ""
        dialog.open()
    }
    function submit() {
        if (!dialog.controller)
            return
        dialog.controller.arguments = argumentsField.text
        const form = formBox.currentIndex
        const ok = dialog.fileMode
            ? dialog.controller.createFileLink(targetField.text.trim(), form,
                                               storeBox.checked ? LinkDialogs.DocumentStorage
                                                                : LinkDialogs.ExternalStorage, 0)
            : dialog.controller.createWebsiteLink(targetField.text, form, 0)
        if (ok)
            dialog.close()
    }

    Component.onCompleted: {
        if (!commands)
            return
        commands.setHandler("link.file", function (arg) { dialog.begin(true, arg) })
        commands.setHandler("link.website", function (arg) { dialog.begin(false, arg) })
    }
    onOpened: targetField.forceActiveFocus()

    Connections {
        target: dialog.controller
        function onRefused(reason) { dialog.error = reason }
    }

    contentItem: ColumnLayout {
        spacing: 8
        Label {
            text: dialog.fileMode ? qsTr("Fichier") : qsTr("Adresse du site (http ou https)")
        }
        RowLayout {
            Layout.fillWidth: true
            TextField {
                id: targetField
                objectName: "linkTarget"
                Layout.fillWidth: true
                selectByMouse: true
                onTextEdited: dialog.error = ""
                onAccepted: dialog.submit()
            }
            Button {
                objectName: "linkBrowse"
                visible: dialog.fileMode
                text: qsTr("Parcourir…")
                onClicked: fileChooser.open()
            }
        }
        Label { text: qsTr("Forme du lien") }
        ComboBox {
            id: formBox
            objectName: "linkForm"
            Layout.fillWidth: true
            // L'ordre suit LinkDialogs::Form.
            model: [qsTr("Texte"), qsTr("Icône"), qsTr("Objet d'action"), qsTr("Objet sélectionné sur la page")]
        }
        CheckBox {
            id: storeBox
            objectName: "linkStore"
            visible: dialog.fileMode
            text: qsTr("Ranger le fichier dans le paperboard")
        }
        Label {
            visible: dialog.fileMode
            text: qsTr("Paramètres de ligne de commande (gardés, jamais passés)")
        }
        TextField {
            id: argumentsField
            objectName: "linkArguments"
            visible: dialog.fileMode
            Layout.fillWidth: true
            selectByMouse: true
        }
        Label {
            objectName: "linkError"
            Layout.fillWidth: true
            visible: dialog.error !== ""
            text: dialog.error
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            color: "#E57373"
        }
    }
    footer: DialogButtonBox {
        Button {
            objectName: "linkCreate"
            text: qsTr("Créer le lien")
            onClicked: dialog.submit()
        }
        Button {
            objectName: "linkCancel"
            text: qsTr("Annuler")
            onClicked: dialog.close()
        }
    }

    FileDialog {
        id: fileChooser
        objectName: "linkFileChooser"
        title: qsTr("Choisir le fichier du lien")
        fileMode: FileDialog.OpenFile
        // Le chemin local décodé (`#`, `%23`… dans le nom), jamais le texte de l'URL.
        onAccepted: targetField.text = dialog.controller ? dialog.controller.localPath(selectedFile)
                                                         : selectedFile.toString().replace(/^file:\/\/(\/(?=[A-Za-z]:))?/, "")
    }
}
