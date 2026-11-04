import QtQuick
import QtQuick.Dialogs
import ActCraft.Ui

// Coquille (L5-T2) : dialogues de fichiers de l'export, de l'import et de l'insertion (choisir un fichier, puis
// appeler FileExchange ou Insertion). Chaque boîte posée sur la vue
// émet popupOpened et popupClosed (contrat du lot 4).
Item {
    id: root
    visible: false

    required property FileExchange exchange
    required property Insertion insertion
    signal popupOpened()
    signal popupClosed()

    function chooseImage() {
        insertImageDialog.currentFolder = root.exchange.document.defaultFolderUrl(true)
        insertImageDialog.open()
    }
    function chooseMedia() {
        insertMediaDialog.currentFolder = root.exchange.document.defaultFolderUrl(true)
        insertMediaDialog.open()
    }
    // Nom proposé : le titre du paperboard (ses caractères interdits dans un nom de fichier deviennent « _ »),
    // dans le dossier des documents.
    function suggestion(suffix) {
        const base = root.exchange.document.suggestedFileName.replace(/\.actcraft$/, "")
        return root.exchange.document.defaultFolderUrl(true) + "/" + base.replace(/[\\\/:*?"<>|]/g, "_") + suffix
    }
    function chooseExportPdf() {
        exportPdfDialog.currentFile = root.suggestion(".pdf")
        exportPdfDialog.open()
    }
    function chooseExportPageImage() {
        exportImageDialog.currentFile = root.suggestion(".png")
        exportImageDialog.open()
    }
    function chooseImportPdf() {
        importPdfDialog.currentFolder = root.exchange.document.defaultFolderUrl(true)
        importPdfDialog.open()
    }
    function chooseImportPowerpoint() {
        importPowerpointDialog.currentFolder = root.exchange.document.defaultFolderUrl(true)
        importPowerpointDialog.open()
    }

    // Les scénarios et les commandes avec argument n'ouvrent jamais ces boîtes (Main.qml) : l'URL va droit à
    // FileExchange.
    FileDialog {
        id: insertImageDialog
        objectName: "insertImageDialog"
        title: qsTr("Insérer une image")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Images (*.png *.jpg *.jpeg *.bmp *.gif *.tif *.tiff *.svg)"), qsTr("Tous les fichiers (*)")]
        onAccepted: root.insertion.insertImage(selectedFile)
        onVisibleChanged: visible ? root.popupOpened() : root.popupClosed()
    }
    FileDialog {
        id: insertMediaDialog
        objectName: "insertMediaDialog"
        title: qsTr("Insérer un média")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Sons et vidéos (*.mp3 *.wav *.ogg *.oga *.m4a *.aac *.flac *.opus *.mp4 *.m4v *.mov *.avi *.webm *.mkv *.mpg *.mpeg *.wmv)"),
                      qsTr("Tous les fichiers (*)")]
        onAccepted: root.insertion.insertMedia(selectedFile)
        onVisibleChanged: visible ? root.popupOpened() : root.popupClosed()
    }
    FileDialog {
        id: exportPdfDialog
        objectName: "exportPdfDialog"
        title: qsTr("Exporter en PDF")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "pdf"
        nameFilters: [qsTr("Documents PDF (*.pdf)")]
        onAccepted: root.exchange.exportPdf(selectedFile)
        onVisibleChanged: visible ? root.popupOpened() : root.popupClosed()
    }
    FileDialog {
        id: exportImageDialog
        objectName: "exportImageDialog"
        title: qsTr("Exporter la page en image")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "png"
        nameFilters: [qsTr("Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff)")]
        onAccepted: root.exchange.exportPageImage(selectedFile)
        onVisibleChanged: visible ? root.popupOpened() : root.popupClosed()
    }
    FileDialog {
        id: importPdfDialog
        objectName: "importPdfDialog"
        title: qsTr("Importer un PDF")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Documents PDF (*.pdf)")]
        onAccepted: root.exchange.importPdf(selectedFile)
        onVisibleChanged: visible ? root.popupOpened() : root.popupClosed()
    }
    FileDialog {
        id: importPowerpointDialog
        objectName: "importPowerpointDialog"
        title: qsTr("Importer une présentation en images")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Présentations (*.pptx *.ppt *.ppsx *.odp)")]
        onAccepted: root.exchange.importPowerpointImages(selectedFile)
        onVisibleChanged: visible ? root.popupOpened() : root.popupClosed()
    }
}
