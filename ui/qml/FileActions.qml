import QtQuick
import QtQuick.Dialogs
import ActCraft.Ui

// Commandes de fichier (F-700) : avec une URL, sans dialogue ; sans URL, le dialogue de fichier (natif ;
// content:// sous Android). Tout ce qui remplace un document modifié demande d'abord de l'enregistrer.
// Pas d'onglets avant le lot 5 : Nouveau et Ouvrir remplacent le document affiché ; Fermer laisse un paperboard
// vierge « Sans titre » (F-656).
Item {
    id: actions
    objectName: "fileActions"
    required property DocumentController document
    signal popupOpened()
    signal popupClosed()
    property var pending: null    // action à reprendre après « enregistrer ? »
    property var afterSave: null  // action à reprendre après un enregistrement réussi
    property bool quitting: false

    function guard(action) {
        if (!actions.document.modified) { action(); return }
        actions.pending = action
        saveChanges.documentTitle = actions.document.title
        saveChanges.open()
    }
    function newDocument(w, h) { guard(function () { actions.document.newDocument(w, h) }) }
    function askSize() { sizeDialog.open() }
    function open(argument) {
        const url = actions.document.urlFromArgument(argument)
        if (url.toString() === "") {
            guard(function () {
                // Le dialogue crée le dossier par défaut en s'ouvrant (jamais ailleurs).
                openDialog.currentFolder = actions.document.defaultFolderUrl(true)
                openDialog.open()
            })
        } else {
            guard(function () { actions.document.openUrl(url) })
        }
    }
    function save() {
        if (actions.document.fileUrl.toString() === "") saveAs(undefined)
        else actions.document.saveToUrl(actions.document.fileUrl)
    }
    function saveAs(argument) {
        const url = actions.document.urlFromArgument(argument)
        if (url.toString() !== "") { actions.document.saveToUrl(url); return }
        const folder = actions.document.defaultFolderUrl(true)
        saveDialog.currentFolder = folder
        saveDialog.selectedFile = folder + "/" + encodeURIComponent(actions.document.suggestedFileName)
        saveDialog.open()
    }
    function close() { guard(function () { actions.document.newDocument(1024, 768) }) }
    function quit() { guard(function () { actions.quitting = true; Qt.quit() }) }

    FileDialog {
        id: openDialog
        objectName: "openDialog"
        title: qsTr("Ouvrir un paperboard")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Paperboards (*.actcraft *.flipchart)"), qsTr("Tous les fichiers (*)")]
        onAccepted: actions.document.openUrl(selectedFile)
    }
    FileDialog {
        id: saveDialog
        objectName: "saveDialog"
        title: qsTr("Enregistrer le paperboard sous")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "actcraft"
        nameFilters: [qsTr("Paperboards ActCraft (*.actcraft)")]
        onAccepted: actions.document.saveToUrl(selectedFile)
        // « Enregistrer sous » abandonné après « Enregistrer les modifications ? » : l'action en attente est oubliée.
        onRejected: actions.afterSave = null
    }
    SaveChangesDialog {
        id: saveChanges
        onOpened: actions.popupOpened()
        onClosed: actions.popupClosed()
        onSaveRequested: { actions.afterSave = actions.pending; actions.pending = null; actions.save() }
        onDiscardRequested: { const a = actions.pending; actions.pending = null; if (a) a() }
        onRejected: actions.pending = null
    }
    PageSizeDialog {
        id: sizeDialog
        onOpened: actions.popupOpened()
        onClosed: actions.popupClosed()
        onSizeAccepted: (w, h) => actions.newDocument(w, h)
    }
    Connections {
        target: actions.document
        function onSaveFinished(ok, message) {
            const a = actions.afterSave
            actions.afterSave = null
            if (ok && a) a()
        }
    }
}
