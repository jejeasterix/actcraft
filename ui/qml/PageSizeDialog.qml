import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// « Taille de paperboard personnalisée » (F-700) : largeur × hauteur en pixels.
Dialog {
    id: dialog
    objectName: "pageSizeDialog"
    signal sizeAccepted(real width, real height)
    title: qsTr("Taille de paperboard personnalisée")
    modal: true
    anchors.centerIn: Overlay.overlay
    standardButtons: Dialog.Ok | Dialog.Cancel

    function acceptSize(w, h) { widthBox.value = w; heightBox.value = h; dialog.accept() }
    onAccepted: dialog.sizeAccepted(widthBox.value, heightBox.value)

    GridLayout {
        columns: 2
        Label { text: qsTr("Largeur (pixels)") }
        SpinBox { id: widthBox; objectName: "pageWidthBox"; from: 200; to: 8000; value: 1024; editable: true }
        Label { text: qsTr("Hauteur (pixels)") }
        SpinBox { id: heightBox; objectName: "pageHeightBox"; from: 200; to: 8000; value: 768; editable: true }
    }
}
