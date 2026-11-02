import QtQuick
import ActCraft.Ui

// Coquille (L5-T2) : dialogues de fichiers de l'export, de l'import et de l'insertion (choisir un fichier, puis
// appeler FileExchange ou Insertion). Les corps viennent de L5-T25 et L5-T30 ; chaque boîte posée sur la vue
// émet popupOpened et popupClosed (contrat du lot 4).
Item {
    id: root
    visible: false

    required property FileExchange exchange
    required property Insertion insertion
    signal popupOpened()
    signal popupClosed()

    function chooseImage() {
    }
    function chooseMedia() {
    }
    function chooseExportPdf() {
    }
    function chooseExportPageImage() {
    }
    function chooseImportPdf() {
    }
    function chooseImportPowerpoint() {
    }
}
