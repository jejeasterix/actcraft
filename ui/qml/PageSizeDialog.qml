import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// « Taille de paperboard personnalisée » (F-700, D16) : taille d'un cadre en pixels et nombre de cadres.
// Page = nombre de cadres × taille d'un cadre ; un cadre fait au moins dialog.minFrame pixels par côté (amendement 5).
Dialog {
    id: dialog
    objectName: "pageSizeDialog"
    signal sizeAccepted(real width, real height, int across, int down)
    title: qsTr("Taille de paperboard personnalisée")
    modal: true
    anchors.centerIn: Overlay.overlay
    standardButtons: Dialog.Ok | Dialog.Cancel

    // Contrôleur du document : source unique du plafond d'un côté et du plus petit cadre.
    property var document: null
    readonly property real maxSide: document ? document.maxPageSide : 0
    readonly property real minFrame: document ? document.minFrameSide : 0
    readonly property bool framed: framesAcrossBox.value > 0 || framesDownBox.value > 0

    function acceptSize(w, h) {
        framesAcrossBox.value = 0; framesDownBox.value = 0
        widthBox.value = w; heightBox.value = h
        dialog.accept()
    }
    // Les nombres de cadres d'abord : le minimum par cadre (minFrame) s'applique ensuite aux deux tailles.
    function acceptFramedSize(w, h, across, down) {
        framesAcrossBox.value = across; framesDownBox.value = down
        widthBox.value = w; heightBox.value = h
        dialog.accept()
    }
    onAccepted: dialog.sizeAccepted(widthBox.value, heightBox.value, framesAcrossBox.value, framesDownBox.value)

    GridLayout {
        columns: 2
        Label { text: qsTr("Largeur d'un cadre (pixels)") }
        SpinBox { id: widthBox; objectName: "pageWidthBox"; from: dialog.framed ? dialog.minFrame : 200; to: Math.max(from, Math.floor(dialog.maxSide / Math.max(1, framesAcrossBox.value))); value: 1024; editable: true }
        Label { text: qsTr("Hauteur d'un cadre (pixels)") }
        SpinBox { id: heightBox; objectName: "pageHeightBox"; from: dialog.framed ? dialog.minFrame : 200; to: Math.max(from, Math.floor(dialog.maxSide / Math.max(1, framesDownBox.value))); value: 768; editable: true }
        Label { text: qsTr("Cadres en largeur") }
        SpinBox { id: framesAcrossBox; objectName: "pageFramesAcrossBox"; from: 0; to: Math.min(40, Math.floor(dialog.maxSide / Math.max(widthBox.value, dialog.minFrame))); value: 0; editable: true }
        Label { text: qsTr("Cadres en hauteur") }
        SpinBox { id: framesDownBox; objectName: "pageFramesDownBox"; from: 0; to: Math.min(53, Math.floor(dialog.maxSide / Math.max(heightBox.value, dialog.minFrame))); value: 0; editable: true }
    }
}
