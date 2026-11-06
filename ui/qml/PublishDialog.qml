import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

// Diffusion protégée (L8-T31, F-680, S18) : informations de l'auteur et quatre interdictions ; « Diffuser… »
// écrit un nouveau fichier qui porte les règles (DocumentController.publishToUrl). Le document ouvert reste
// intact, sans règles, sur son fichier. Courtoisie sur un format ouvert : aucun chiffrement.
Item {
    id: root
    visible: false
    objectName: "publishDialog"
    property var commands: null
    // Contrôleur du document ; à défaut, celui de la fenêtre (appDocument).
    property var document: null
    readonly property var target: document ? document : (typeof appDocument !== "undefined" ? appDocument : null)
    property string lastMessage: ""

    function rules() {
        return { noCopy: copyBox.checked, noSave: saveBox.checked, noPrint: printBox.checked,
                 noCamera: cameraBox.checked, author: authorField.text.trim() }
    }
    function publishTo(url) {
        if (!root.target || url.toString() === "")
            return
        const r = rules()
        root.target.publishToUrl(url, r.noCopy, r.noSave, r.noPrint, r.noCamera, r.author)
    }

    Component.onCompleted: {
        if (!commands)
            return
        // Argument (scénarios) : chemin du fichier diffusé, règles de la boîte ; sinon la boîte s'ouvre.
        commands.setHandler("file.publish", function (arg) {
            if (arg !== undefined && arg !== null && arg.toString() !== "" && root.target)
                root.publishTo(root.target.urlFromArgument(arg))
            else
                dialog.open()
        })
    }

    Connections {
        target: root.target
        ignoreUnknownSignals: true
        function onPublishFinished(ok, message) {
            root.lastMessage = ok ? qsTr("Copie protégée enregistrée.") : message
            console.info("ACTCRAFT DIFFUSE " + (ok ? "OK" : "ECHEC"))
        }
    }

    Dialog {
        id: dialog
        objectName: "publishDialogBox"
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: qsTr("Diffuser en protégé")
        standardButtons: Dialog.Cancel

        ColumnLayout {
            spacing: 8
            Label { text: qsTr("Informations de l'auteur") }
            TextField {
                id: authorField
                objectName: "publishAuthor"
                Layout.preferredWidth: 320
                placeholderText: qsTr("Nom, établissement, contact")
            }
            Label { text: qsTr("Interdire dans la copie diffusée :") }
            CheckBox { id: copyBox; objectName: "publishNoCopy"; checked: true; text: qsTr("La copie de pages et d'objets") }
            CheckBox { id: saveBox; objectName: "publishNoSave"; checked: true; text: qsTr("L'enregistrement et l'export") }
            CheckBox { id: printBox; objectName: "publishNoPrint"; checked: true; text: qsTr("L'impression") }
            CheckBox { id: cameraBox; objectName: "publishNoCamera"; checked: true; text: qsTr("La caméra") }
            Label {
                Layout.preferredWidth: 320
                wrapMode: Text.WordWrap
                text: qsTr("Le document ouvert n'est pas modifié : la copie protégée est un nouveau fichier.")
            }
            Button {
                objectName: "publishButton"
                text: qsTr("Diffuser…")
                onClicked: { dialog.close(); fileDialog.open() }
            }
        }
    }

    FileDialog {
        id: fileDialog
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Document ActCraft (*.actcraft)")]
        currentFolder: root.target ? root.target.defaultFolderUrl(false) : ""
        onAccepted: root.publishTo(selectedFile)
    }
}
