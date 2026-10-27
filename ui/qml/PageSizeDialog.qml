import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// « Taille de paperboard personnalisée » (F-700, D16) : taille d'un cadre en pixels et nombre de cadres.
// Page = nombre de cadres × taille d'un cadre ; un cadre fait au moins 500 pixels par côté (amendement 5).
Dialog {
    id: dialog
    objectName: "pageSizeDialog"
    signal sizeAccepted(real width, real height, int across, int down)
    title: qsTr("Taille de paperboard personnalisée")
    modal: true
    anchors.centerIn: Overlay.overlay
    standardButtons: Dialog.Ok | Dialog.Cancel

    readonly property bool framed: framesAcrossBox.value > 0 || framesDownBox.value > 0

    function acceptSize(w, h) {
        framesAcrossBox.value = 0; framesDownBox.value = 0
        widthBox.value = w; heightBox.value = h
        dialog.accept()
    }
    // Les nombres de cadres d'abord : le minimum de 500 par cadre s'applique ensuite aux deux tailles.
    function acceptFramedSize(w, h, across, down) {
        framesAcrossBox.value = across; framesDownBox.value = down
        widthBox.value = w; heightBox.value = h
        dialog.accept()
    }
    onAccepted: dialog.sizeAccepted(widthBox.value, heightBox.value, framesAcrossBox.value, framesDownBox.value)

    GridLayout {
        columns: 2
        Label { text: qsTr("Largeur d'un cadre (pixels)") }
        SpinBox { id: widthBox; objectName: "pageWidthBox"; from: dialog.framed ? 500 : 200; to: 8000; value: 1024; editable: true }
        Label { text: qsTr("Hauteur d'un cadre (pixels)") }
        SpinBox { id: heightBox; objectName: "pageHeightBox"; from: dialog.framed ? 500 : 200; to: 8000; value: 768; editable: true }
        Label { text: qsTr("Cadres en largeur") }
        SpinBox { id: framesAcrossBox; objectName: "pageFramesAcrossBox"; from: 0; to: 40; value: 0; editable: true }
        Label { text: qsTr("Cadres en hauteur") }
        SpinBox { id: framesDownBox; objectName: "pageFramesDownBox"; from: 0; to: 53; value: 0; editable: true }
    }
}
