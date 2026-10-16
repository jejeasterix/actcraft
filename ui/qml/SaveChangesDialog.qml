import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// « Enregistrer les modifications ? » (F-700 : Nouveau, Ouvrir, Fermer, Quitter) : Enregistrer, Ne pas
// enregistrer, Annuler. Le titre vient d'un fichier : texte brut, jamais interprété comme du balisage.
Dialog {
    id: dialog
    objectName: "saveChangesDialog"
    property string documentTitle: ""
    signal saveRequested()
    signal discardRequested()
    title: qsTr("Enregistrer les modifications ?")
    modal: true
    width: 460
    anchors.centerIn: Overlay.overlay
    closePolicy: Popup.CloseOnEscape

    function discard() { dialog.close(); dialog.discardRequested() }
    function cancel() { dialog.reject() }
    function save() { dialog.close(); dialog.saveRequested() }

    contentItem: Label {
        objectName: "saveChangesText"
        text: qsTr("Le paperboard « %1 » a été modifié. Enregistrer les modifications ?").arg(dialog.documentTitle)
        textFormat: Text.PlainText
        wrapMode: Text.Wrap
    }
    footer: DialogButtonBox {
        Button { objectName: "saveChangesSave"; text: qsTr("Enregistrer"); onClicked: dialog.save() }
        Button { objectName: "saveChangesDiscard"; text: qsTr("Ne pas enregistrer"); onClicked: dialog.discard() }
        Button { objectName: "saveChangesCancel"; text: qsTr("Annuler"); onClicked: dialog.cancel() }
    }
}
